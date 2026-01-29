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
    
    // MSAA設定を取得
    bool msaaEnabled = true;
    auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
    if (defaultScene)
    {
        auto pipeline = defaultScene->GetDefaultPipelineManager();
        if (pipeline)
            msaaEnabled = pipeline->GetAASettings().msaaEnabled;
    }
    
    // MSAA設定に応じてPSOを選択
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

    if (msaaEnabled)
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

    // 射影行列 - 左手座標系に統一
    XMMATRIX proj = XMMatrixPerspectiveFovLH(
        context.Camera->GetFOV(),
        context.Camera->GetAspect(),
        0.3f,
        1000.0f // Skyboxには近距離で十分
    );

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
