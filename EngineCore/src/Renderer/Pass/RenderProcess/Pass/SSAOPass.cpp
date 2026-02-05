#include "SSAOPass.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Renderer/Engine.h"
#include "Renderer/Target/RenderTarget.h"
#include "Renderer/Target/DepthStencilTarget.h"
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

    void TransitionToDepthWrite(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> rt)
    {
        if (!rt || !rt->GetResource()) return;
        if (rt->GetCurrentState() == D3D12_RESOURCE_STATE_DEPTH_WRITE) return;
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            rt->GetResource(),
            rt->GetCurrentState(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE);
        cmdList->ResourceBarrier(1, &barrier);
        rt->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }
}

SSAOPass::SSAOPass()
{
    m_ssaoConstants = std::make_shared<ConstantBuffer>(sizeof(SSAOConstants));
}

void SSAOPass::Execute(RenderContext& context)
{
    auto depthRT = context.GetRenderTarget(ConstRenderPref::SceneDepth);
    auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
    auto ssaoRT = context.GetRenderTarget(ConstRenderPref::SSAOBuffer);

    if (!depthRT || !normalRT || !ssaoRT) return;
    if (!depthRT->GetSRVHandle() || !normalRT->GetSRVHandle()) return;
    if (!ssaoRT->GetRTVHandle().ptr) return;

    auto cmdList = context.CommandList;
    TransitionToSRV(cmdList, depthRT);
    TransitionToSRV(cmdList, normalRT);
    TransitionToRTV(cmdList, ssaoRT);

    const UINT width = ssaoRT->GetWidth();
    const UINT height = ssaoRT->GetHeight();

    SSAOConstants* cb = m_ssaoConstants->GetPtr<SSAOConstants>();
    DirectX::XMMATRIX view = context.Camera->GetViewMatrix();
    DirectX::XMMATRIX proj = context.GetProjectionMatrix();
    DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(nullptr, proj);

    DirectX::XMStoreFloat4x4(&cb->View, view);
    DirectX::XMStoreFloat4x4(&cb->InvProjection, invProj);
    DirectX::XMStoreFloat4x4(&cb->Projection, proj);

    const float nearZ = context.Camera->GetNearPlane();
    const float farZ = context.Camera->GetFarPlane();
    cb->ProjectionParams = DirectX::XMFLOAT4(farZ, 1.0f / farZ, static_cast<float>(width), static_cast<float>(height));
    cb->Radius = m_radius;
    cb->Bias = m_bias;
    cb->Power = m_power;
    cb->Enable = m_enabled ? 1.0f : 0.0f;

    const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    cmdList->ClearRenderTargetView(ssaoRT->GetRTVHandle(), clearColor, 0, nullptr);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = ssaoRT->GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("SSAOPass")->Get());
    cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("SSAO_Default")->Get());

    ID3D12DescriptorHeap* heaps[] = { g_Engine->GetDescriptorHeap()->GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, m_ssaoConstants->GetAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, depthRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    TransitionToSRV(cmdList, ssaoRT);
    // SkyboxPass および次フレームの GeometryPass が深度バッファを書き込み用に使うため、SRV から戻す
    TransitionToDepthWrite(cmdList, depthRT);
}
