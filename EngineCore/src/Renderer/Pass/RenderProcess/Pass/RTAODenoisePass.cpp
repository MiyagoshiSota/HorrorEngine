#include "RTAODenoisePass.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Renderer/Engine.h"
#include "Renderer/Target/RenderTarget.h"
#include <d3dx12.h>

namespace
{
    void TransitionToSRV(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> rt)
    {
        if (!rt || !rt->GetResource()) return;
        if (rt->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) return;
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            rt->GetResource(),
            rt->GetCurrentState(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);
        rt->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    void TransitionToRTV(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> rt)
    {
        if (!rt || !rt->GetResource()) return;
        if (rt->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET) return;
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            rt->GetResource(),
            rt->GetCurrentState(),
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &barrier);
        rt->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
}

RTAODenoisePass::RTAODenoisePass()
{
    m_constants = std::make_shared<ConstantBuffer>(sizeof(RTAODenoiseConstants));
}

void RTAODenoisePass::Execute(RenderContext& context)
{
    auto aoRaw = context.GetRenderTarget(ConstRenderPref::RTAORaw);
    auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
    auto worldPosRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);
    auto outputRT = context.GetRenderTarget(ConstRenderPref::SSAOBuffer);

    if (!aoRaw || !normalRT || !worldPosRT || !outputRT) return;
    if (!aoRaw->GetSRVHandle() || !normalRT->GetSRVHandle() || !worldPosRT->GetSRVHandle()) return;
    if (!outputRT->GetRTVHandle().ptr) return;

    auto cmdList = context.CommandList;
    TransitionToSRV(cmdList, aoRaw);
    TransitionToSRV(cmdList, normalRT);
    TransitionToSRV(cmdList, worldPosRT);
    TransitionToRTV(cmdList, outputRT);

    const UINT width = outputRT->GetWidth();
    const UINT height = outputRT->GetHeight();

    RTAODenoiseConstants* cb = m_constants->GetPtr<RTAODenoiseConstants>();
    DirectX::XMStoreFloat3(&cb->CameraPosition, context.Camera->GetEyePos());
    cb->DepthSigma = m_depthSigma;
    cb->NormalSigma = m_normalSigma;
    cb->InvScreenSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
    cb->Padding0 = 0.0f;

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = outputRT->GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("RTAODenoisePass")->Get());
    cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("RTAO_Denoise")->Get());

    ID3D12DescriptorHeap* heaps[] = { g_Engine->GetDescriptorHeap()->GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, m_constants->GetAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, aoRaw->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(3, worldPosRT->GetSRVHandle()->gpuHandle);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    TransitionToSRV(cmdList, outputRT);
}
