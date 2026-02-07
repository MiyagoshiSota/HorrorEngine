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

    // RTV に遷移。UAV 専用リソース（例: RTAO 出力）の場合はスキップする。
    void TransitionToRTV(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> rt, ID3D12Resource* excludeUavOnlyResource = nullptr)
    {
        if (!rt || !rt->GetResource()) return;
        if (excludeUavOnlyResource && rt->GetResource() == excludeUavOnlyResource) return;
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
    auto tempRT = context.GetRenderTarget(ConstRenderPref::RTAODenoiseTemp);

    if (!aoRaw || !outputRT) return;
    if (!aoRaw->GetSRVHandle() || !outputRT->GetRTVHandle().ptr) return;
    ID3D12Resource* aoRawResource = aoRaw->GetResource();
    if (aoRawResource && (outputRT->GetResource() == aoRawResource || (tempRT && tempRT->GetResource() == aoRawResource)))
        return;

    auto cmdList = context.CommandList;
    const UINT width = outputRT->GetWidth();
    const UINT height = outputRT->GetHeight();

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    D3D12_RECT scissor = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    ID3D12DescriptorHeap* heaps[] = { g_Engine->GetDescriptorHeap()->GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    switch (m_denoiseMode)
    {
    case RTAODenoiseMode::Off:
    {
        TransitionToSRV(cmdList, aoRaw);
        TransitionToRTV(cmdList, outputRT, aoRawResource);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = outputRT->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("RTAODenoiseCopyPass")->Get());
        cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("RTAO_Copy")->Get());
        cmdList->SetGraphicsRootDescriptorTable(0, aoRaw->GetSRVHandle()->gpuHandle);
        cmdList->DrawInstanced(3, 1, 0, 0);
        TransitionToSRV(cmdList, outputRT);
        return;
    }
    case RTAODenoiseMode::Bilateral:
    {
        if (!normalRT || !worldPosRT || !normalRT->GetSRVHandle() || !worldPosRT->GetSRVHandle()) return;
        TransitionToSRV(cmdList, aoRaw);
        TransitionToSRV(cmdList, normalRT);
        TransitionToSRV(cmdList, worldPosRT);
        TransitionToRTV(cmdList, outputRT, aoRawResource);

        RTAODenoiseConstants* cb = m_constants->GetPtr<RTAODenoiseConstants>();
        DirectX::XMStoreFloat3(&cb->CameraPosition, context.Camera->GetEyePos());
        cb->DepthSigma = m_depthSigma;
        cb->NormalSigma = m_normalSigma;
        cb->InvScreenSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
        cb->BlurDirection = DirectX::XMFLOAT2(0.0f, 0.0f);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvBilateral = outputRT->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtvBilateral, FALSE, nullptr);
        cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("RTAODenoiseBilateralPass")->Get());
        cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("RTAO_Denoise")->Get());
        cmdList->SetGraphicsRootConstantBufferView(0, m_constants->GetAddress());
        cmdList->SetGraphicsRootDescriptorTable(1, aoRaw->GetSRVHandle()->gpuHandle);
        cmdList->SetGraphicsRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
        cmdList->SetGraphicsRootDescriptorTable(3, worldPosRT->GetSRVHandle()->gpuHandle);
        cmdList->DrawInstanced(3, 1, 0, 0);
        TransitionToSRV(cmdList, outputRT);
        return;
    }
    case RTAODenoiseMode::Separable:
    {
        if (!normalRT || !worldPosRT || !tempRT) return;
        if (!normalRT->GetSRVHandle() || !worldPosRT->GetSRVHandle() || !tempRT->GetRTVHandle().ptr) return;

        RTAODenoiseConstants* cb = m_constants->GetPtr<RTAODenoiseConstants>();
        DirectX::XMStoreFloat3(&cb->CameraPosition, context.Camera->GetEyePos());
        cb->DepthSigma = m_depthSigma;
        cb->NormalSigma = m_normalSigma;
        cb->InvScreenSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);

        // Horizontal: RTAORaw -> RTAODenoiseTemp
        TransitionToSRV(cmdList, aoRaw);
        TransitionToSRV(cmdList, normalRT);
        TransitionToSRV(cmdList, worldPosRT);
        TransitionToRTV(cmdList, tempRT, aoRawResource);
        cb->BlurDirection = DirectX::XMFLOAT2(1.0f, 0.0f);

        // 横方向
        D3D12_CPU_DESCRIPTOR_HANDLE rtvTemp = tempRT->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtvTemp, FALSE, nullptr);
        cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("RTAODenoiseSeparablePass")->Get());
        cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("RTAO_Denoise")->Get());
        cmdList->SetGraphicsRootConstantBufferView(0, m_constants->GetAddress());
        cmdList->SetGraphicsRootDescriptorTable(1, aoRaw->GetSRVHandle()->gpuHandle);
        cmdList->SetGraphicsRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
        cmdList->SetGraphicsRootDescriptorTable(3, worldPosRT->GetSRVHandle()->gpuHandle);
        cmdList->DrawInstanced(3, 1, 0, 0);

        // Vertical: RTAODenoiseTemp -> SSAOBuffer
        TransitionToSRV(cmdList, tempRT);
        TransitionToRTV(cmdList, outputRT, aoRawResource);
        cb->BlurDirection = DirectX::XMFLOAT2(0.0f, 1.0f);

        // 縦方向
        D3D12_CPU_DESCRIPTOR_HANDLE rtvOutput = outputRT->GetRTVHandle();
        cmdList->OMSetRenderTargets(1, &rtvOutput, FALSE, nullptr);
        cmdList->SetGraphicsRootDescriptorTable(1, tempRT->GetSRVHandle()->gpuHandle);
        cmdList->SetGraphicsRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
        cmdList->SetGraphicsRootDescriptorTable(3, worldPosRT->GetSRVHandle()->gpuHandle);
        cmdList->DrawInstanced(3, 1, 0, 0);

        TransitionToSRV(cmdList, outputRT);
        return;
    }
    case RTAODenoiseMode::ATrous:
    {
        if (!normalRT || !worldPosRT || !tempRT) return;
        if (!normalRT->GetSRVHandle() || !worldPosRT->GetSRVHandle() || !tempRT->GetRTVHandle().ptr) return;

        RTAODenoiseConstants* cb = m_constants->GetPtr<RTAODenoiseConstants>();
        DirectX::XMStoreFloat3(&cb->CameraPosition, context.Camera->GetEyePos());
        cb->DepthSigma = m_depthSigma;
        cb->NormalSigma = m_normalSigma;
        cb->InvScreenSize = DirectX::XMFLOAT2(1.0f / width, 1.0f / height);
        cb->BlurDirection = DirectX::XMFLOAT2(0.0f, 0.0f);

        static const float kSteps[] = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
        const int kNumPasses = 5;
        // ping-pong 間でのみ書き込み先を切り替える（aoRaw は常に読み取り専用）
        std::shared_ptr<ITargetBase> pingRT = tempRT;    // 一時バッファ
        std::shared_ptr<ITargetBase> pongRT = outputRT;  // 出力バッファ

        for (int i = 0; i < kNumPasses; ++i)
        {
            std::shared_ptr<ITargetBase> srcRT;
            std::shared_ptr<ITargetBase> dstRT;

            if (i == 0)
            {
                // 1パス目: RTAORaw -> pingRT
                srcRT = aoRaw;
                dstRT = pingRT;
            }
            else
            {
                // 2パス目以降: ping / pong 間で交互に書き込み
                const bool odd = (i & 1) == 1;
                srcRT = odd ? pingRT : pongRT;
                dstRT = odd ? pongRT : pingRT;
            }

            cb->StepSize = kSteps[i];
            TransitionToSRV(cmdList, srcRT);
            TransitionToSRV(cmdList, normalRT);
            TransitionToSRV(cmdList, worldPosRT);
            TransitionToRTV(cmdList, dstRT, aoRawResource);

            D3D12_CPU_DESCRIPTOR_HANDLE rtvDst = dstRT->GetRTVHandle();
            cmdList->OMSetRenderTargets(1, &rtvDst, FALSE, nullptr);
            cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("RTAODenoiseATrousPass")->Get());
            cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("RTAO_Denoise")->Get());
            cmdList->SetGraphicsRootConstantBufferView(0, m_constants->GetAddress());
            cmdList->SetGraphicsRootDescriptorTable(1, srcRT->GetSRVHandle()->gpuHandle);
            cmdList->SetGraphicsRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
            cmdList->SetGraphicsRootDescriptorTable(3, worldPosRT->GetSRVHandle()->gpuHandle);
            cmdList->DrawInstanced(3, 1, 0, 0);
        }

        // 最終結果が outputRT でない場合のみ、コピーで揃える
        std::shared_ptr<ITargetBase> finalSrcRT =
            ((kNumPasses - 1) & 1) == 0 ? pingRT : pongRT; // 最後に書き込まれた方

        if (finalSrcRT != outputRT)
        {
            TransitionToSRV(cmdList, finalSrcRT);
            TransitionToRTV(cmdList, outputRT, aoRawResource);
            D3D12_CPU_DESCRIPTOR_HANDLE rtvOut = outputRT->GetRTVHandle();
            cmdList->OMSetRenderTargets(1, &rtvOut, FALSE, nullptr);
            cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("RTAODenoiseCopyPass")->Get());
            cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("RTAO_Copy")->Get());
            cmdList->SetGraphicsRootDescriptorTable(0, finalSrcRT->GetSRVHandle()->gpuHandle);
            cmdList->DrawInstanced(3, 1, 0, 0);
        }
        TransitionToSRV(cmdList, outputRT);
        return;
    }
    }
}
