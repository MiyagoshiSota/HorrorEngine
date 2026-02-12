#include "SSRCompositePass.h"
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

SSRCompositePass::SSRCompositePass()
{
    m_compositeConstants = std::make_shared<ConstantBuffer>(sizeof(SSRCompositeConstants));
}

void SSRCompositePass::Execute(RenderContext& context)
{
    auto sceneColorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
    auto ssrRT = context.GetRenderTarget(ConstRenderPref::SSRBuffer);
    auto materialRT = context.GetRenderTarget(ConstRenderPref::GBufferMaterial);
    std::shared_ptr<ITargetBase> destRT = context.GetDestRT();

    if (!sceneColorRT || !ssrRT || !destRT) return;
    if (!sceneColorRT->GetSRVHandle() || !ssrRT->GetSRVHandle()) return;
    if (!destRT->GetRTVHandle().ptr) return;

    auto cmdList = context.CommandList;
    TransitionToSRV(cmdList, sceneColorRT);
    TransitionToSRV(cmdList, ssrRT);
    if (materialRT)
        TransitionToSRV(cmdList, materialRT);
    TransitionToRTV(cmdList, destRT);

    const UINT width = destRT->GetWidth();
    const UINT height = destRT->GetHeight();

    SSRCompositeConstants* cb = m_compositeConstants->GetPtr<SSRCompositeConstants>();
    cb->ReflectionIntensity = m_reflectionIntensity;
    cb->MaxRoughness = m_maxRoughness;
    cb->Padding[0] = cb->Padding[1] = 0.0f;

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("SSRCompositePass")->Get());
    cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("SSRComposite_Default")->Get());

    ID3D12DescriptorHeap* heaps[] = { g_Engine->GetDescriptorHeap()->GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, m_compositeConstants->GetAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, sceneColorRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(2, ssrRT->GetSRVHandle()->gpuHandle);
    if (materialRT && materialRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(3, materialRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(3, sceneColorRT->GetSRVHandle()->gpuHandle);

    D3D12_CPU_DESCRIPTOR_HANDLE destRtv = destRT->GetRTVHandle();
    cmdList->OMSetRenderTargets(1, &destRtv, FALSE, nullptr);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    TransitionToSRV(cmdList, destRT);
}
