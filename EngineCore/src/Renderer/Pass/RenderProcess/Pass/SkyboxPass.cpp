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
    const auto psoName = "SkyboxPass";

    auto rootSig = g_Scene->GetPipelineStateManager()->GetRootSignature(rootSigName);
    auto pso = g_Scene->GetPipelineStateManager()->GetPipelineState(psoName);

    if (!rootSig || !pso)
    {
        return;
    }

    cmdList->SetGraphicsRootSignature(rootSig->Get());
    cmdList->SetPipelineState(pso->Get());

    // MSAAレンダーターゲットに描画（GeometryPassと同じターゲット）
    auto msaaColorRT = context.GetRenderTarget(ConstRenderPref::MSAART);
    auto msaaDepthRT = context.GetRenderTarget(ConstRenderPref::MSAA_Depth);

    if (!msaaColorRT || !msaaDepthRT)
    {
        return;
    }

    // Viewport & Scissor
    auto resourceDesc = msaaColorRT->GetResource()->GetDesc();
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
    auto rtvHandle = msaaColorRT->GetRTVHandle();
    auto dsvHandle = msaaDepthRT->GetDSVHandle();
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // ビュー行列から平行移動成分を除去（回転のみ）
    XMMATRIX view = XMMatrixLookAtRH(
        context.Camera->GetEyePos(),
        context.Camera->GetTargetPos(),
        context.Camera->GetUpward()
    );

    // 平行移動成分を0にする
    view.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    // 射影行列
    XMMATRIX proj = XMMatrixPerspectiveFovRH(
        context.Camera->GetFOV(),
        context.Camera->GetAspect(),
        0.1f,
        10.0f // Skyboxには近距離で十分
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
