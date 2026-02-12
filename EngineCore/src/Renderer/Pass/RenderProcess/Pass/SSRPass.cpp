#include "SSRPass.h"
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
}

SSRPass::SSRPass()
{
    m_ssrConstants = std::make_shared<ConstantBuffer>(sizeof(SSRConstants));
}

void SSRPass::Execute(RenderContext& context)
{
    if (!m_enabled) return;

    auto sceneColorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
    auto sceneDepthRT = context.GetRenderTarget(ConstRenderPref::SceneDepth);
    auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
    auto worldPosRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);
    auto materialRT = context.GetRenderTarget(ConstRenderPref::GBufferMaterial);
    auto ssrRT = context.GetRenderTarget(ConstRenderPref::SSRBuffer);

    if (!sceneColorRT || !sceneDepthRT || !normalRT || !worldPosRT || !ssrRT) return;
    if (!sceneColorRT->GetSRVHandle() || !sceneDepthRT->GetSRVHandle() || !normalRT->GetSRVHandle() ||
        !worldPosRT->GetSRVHandle() || !ssrRT->GetRTVHandle().ptr) return;

    auto cmdList = context.CommandList;
    TransitionToSRV(cmdList, sceneColorRT);
    TransitionToSRV(cmdList, sceneDepthRT);
    TransitionToSRV(cmdList, normalRT);
    TransitionToSRV(cmdList, worldPosRT);
    if (materialRT)
        TransitionToSRV(cmdList, materialRT);
    TransitionToRTV(cmdList, ssrRT);

    const float nearZ = context.Camera->GetNearPlane();
    const float farZ = context.Camera->GetFarPlane();
    const UINT width = ssrRT->GetWidth();
    const UINT height = ssrRT->GetHeight();

    SSRConstants* cb = m_ssrConstants->GetPtr<SSRConstants>();
    DirectX::XMMATRIX view = context.Camera->GetViewMatrix();
    DirectX::XMMATRIX proj = context.GetProjectionMatrix();
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, view);
    DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(nullptr, proj);

    DirectX::XMStoreFloat4x4(&cb->View, view);
    DirectX::XMStoreFloat4x4(&cb->InvView, invView);
    DirectX::XMStoreFloat4x4(&cb->Projection, proj);
    DirectX::XMStoreFloat4x4(&cb->InvProjection, invProj);
    cb->ProjectionParams = DirectX::XMFLOAT4(farZ, 1.0f / farZ, static_cast<float>(width), static_cast<float>(height));
    cb->NearZ = nearZ;
    cb->FarZ = farZ;
    cb->MaxRayDistance = m_maxRayDistance;
    cb->RayStep = m_rayStep;
    cb->MaxSteps = static_cast<float>(m_maxSteps);
    cb->Thickness = m_thickness;
    cb->Enable = m_enabled ? 1.0f : 0.0f;
    cb->Padding[0] = cb->Padding[1] = 0.0f;

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = ssrRT->GetRTVHandle();
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("SSRPass")->Get());
    cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("SSR_Default")->Get());

    ID3D12DescriptorHeap* heaps[] = { g_Engine->GetDescriptorHeap()->GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, m_ssrConstants->GetAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, sceneColorRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(2, sceneDepthRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(3, normalRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(4, worldPosRT->GetSRVHandle()->gpuHandle);
    if (materialRT && materialRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(5, materialRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(5, sceneColorRT->GetSRVHandle()->gpuHandle);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);

    TransitionToSRV(cmdList, ssrRT);
}
