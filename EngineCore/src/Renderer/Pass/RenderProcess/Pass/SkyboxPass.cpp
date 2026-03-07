#include "SkyboxPass.h"

#include "Core/App.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Renderer/Engine.h"
#include "Renderer/Graphics/Buffer/VertexBuffer.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHeap.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include <d3dx12.h>

using namespace DirectX;

bool SkyboxPass::IsEnabled(const RenderContext& context) const
{
    if (!m_enabled)
    {
        return false;
    }

    const auto& skyboxData = context.GetSkyboxData();
    return skyboxData.isValid;
}

void SkyboxPass::Execute(RenderContext& context)
{
    if (!IsEnabled(context))
    {
        return;
    }

    const auto& skyboxData = context.GetSkyboxData();

    auto cmdList = context.CommandList;

    // パイプラインステートとルートシグネチャの設定
    const auto rootSigName = "Skybox_Default";
    
    // G-Buffer パス時は 1x、それ以外は MSAA 設定に応じて PSO を選択
    bool msaaEnabled = true;
    bool useDeferred = (context.GetRenderTarget(ConstRenderPref::GBufferAlbedo) != nullptr);
    if (!useDeferred)
    {
        auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
        if (defaultScene)
        {
            auto pipeline = defaultScene->GetDefaultPipelineManager();
            if (pipeline)
                msaaEnabled = pipeline->GetAASettings().msaaEnabled;
        }
    }
    else
    {
        msaaEnabled = false;
    }
    const char* psoName = msaaEnabled ? "SkyboxPass" : "SkyboxPassNoMSAA";

    auto rootSig = g_Scene->GetPipelineStateManager()->GetRootSignature(rootSigName);
    auto pso = g_Scene->GetPipelineStateManager()->GetPipelineState(psoName);

    if (!rootSig || !pso)
    {
        return;
    }

    cmdList->SetGraphicsRootSignature(rootSig->Get());
    cmdList->SetPipelineState(pso->Get());

    std::shared_ptr<ITargetBase> colorRT;
    std::shared_ptr<ITargetBase> depthRT;
    // G-Buffer + Lighting パス時は SceneColor に LightingPass が書いているのでそこに描画
    if (context.GetRenderTarget(ConstRenderPref::GBufferAlbedo))
    {
        colorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
        depthRT = context.GetRenderTarget(ConstRenderPref::SceneDepth);
    }
    else if (msaaEnabled)
    {
        colorRT = context.GetRenderTarget(ConstRenderPref::MSAART);
        depthRT = context.GetRenderTarget(ConstRenderPref::MSAA_Depth);
    }
    else
    {
        colorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
        depthRT = context.GetRenderTarget(ConstRenderPref::SceneDepth);
    }

    if (!colorRT || !depthRT)
    {
        return;
    }

    // 前パスが SceneColorをSRVのまま終える場合があるため、RTV/DSV へ明示的に遷移する
    auto barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
    RendererUtility::simple_change_target_state(barriers, colorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    RendererUtility::simple_change_target_state(barriers, depthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    if (!barriers->empty())
    {
        cmdList->ResourceBarrier(static_cast<UINT>(barriers->size()), barriers->data());
        colorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        depthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
    }

    // Viewport & Scissor
    auto resourceDesc = colorRT->GetResource()->GetDesc();
    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(resourceDesc.Width);
    viewport.Height = static_cast<float>(resourceDesc.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(resourceDesc.Width);
    scissorRect.bottom = static_cast<LONG>(resourceDesc.Height);

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissorRect);

    // レンダーターゲット設定
    auto rtvHandle = colorRT->GetRTVHandle();
    auto dsvHandle = depthRT->GetDSVHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // ビュー行列から平行移動成分を除去（回転のみ）- 左手座標系に統一
    XMMATRIX view = XMMatrixLookAtLH(
        context.Camera->GetEyePos(),
        context.Camera->GetTargetPos(),
        context.Camera->GetUpward()
    );

    // 平行移動成分を0にする
    view.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    // 射影行列 - TAA有効時はジッター適用済みの投影行列を使用
    XMMATRIX proj = context.GetProjectionMatrix();

    // 定数バッファ更新（RenderContext経由）
    context.UpdateSkyboxConstantBuffer(view * proj);

    // ディスクリプタヒープ設定
    auto heap = g_Engine->GetDescriptorHeap()->GetHeap();
    cmdList->SetDescriptorHeaps(1, &heap);

    // ルートパラメータ設定
    cmdList->SetGraphicsRootConstantBufferView(0, skyboxData.constantBuffer->GetAddress());
    cmdList->SetGraphicsRootDescriptorTable(1, skyboxData.srvHandle->gpuHandle);

    // 描画
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    auto vbView = skyboxData.vertexBuffer->View();
    auto ibView = skyboxData.indexBuffer->View();
    cmdList->IASetVertexBuffers(0, 1, &vbView);
    cmdList->IASetIndexBuffer(&ibView);

    cmdList->DrawIndexedInstanced(skyboxData.indexCount, 1, 0, 0, 0);
}
