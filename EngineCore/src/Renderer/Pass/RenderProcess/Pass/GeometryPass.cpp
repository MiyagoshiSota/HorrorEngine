#include "GeometryPass.h"

#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <cmath>

#include "Core/App.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Model/Model.h"

using namespace DirectX;

// --- 定数定義 ---
static const float kShadowMapSize = 2048.0f;
static const float kShadowDistance = 10000.0f;

// --- ヘルパー関数: シンプルなシャドウマップ用行列計算 ---
void CalculateLightViewProj_Geometry(
    XMVECTOR lightDir,
    XMMATRIX& outViewProj)
{
    // ライトのビュー行列 (View)
    XMVECTOR targetPos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    // ライトの位置を逆算 (原点からライト方向へバックした位置)
    XMVECTOR lightPos = XMVectorAdd(targetPos, XMVectorScale(lightDir, -100.0f));

    // ライトの上方向 
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    
    XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, up);

    // ライトの射影行列
    float sceneWidth = 40.0f;
    float sceneHeight = 40.0f;

    // 原点を中心に左右上下に広げる
    float minX = -sceneWidth / 2.0f;
    float maxX = sceneWidth / 2.0f;
    float minY = -sceneHeight / 2.0f;
    float maxY = sceneHeight / 2.0f;

    // Z深度の範囲 (Near/Far)
    // ライト位置からターゲットまでの距離が100.0fなので、
    // それを挟み込むように十分な範囲を取る
    float minZ = 1.0f;     // Near
    float maxZ = 500.0f;   // Far (奥)
    
    XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, minZ, maxZ);

    // 行列合成
    outViewProj = XMMatrixMultiply(lightView, lightProj);
}

// =========================================================
// Collect
// =========================================================
void GeometryPass::Collect(RenderContext& context)
{
    auto cmdList = context.CommandList;

    // パイプラインステートとルートシグネチャの設定
    auto name = "Geometry_Default";
    auto PSOname = "DefaultPipelinePass";

    cmdList->SetGraphicsRootSignature(g_Scene->GetPipelineStateManager()->GetRootSignature(name)->Get());
    // ※注意: ここのPSOは MSAA Count=8 に設定されている必要があります
    cmdList->SetPipelineState(g_Scene->GetPipelineStateManager()->GetPipelineState(PSOname)->Get());

    m_RenderQueue.clear();
    for (auto& obj : context.GameObjects)
    {
        m_RenderQueue.push_back(obj);
    }

    auto msaaColorRT = context.GetRenderTarget(ConstRenderPref::MSAART);
    auto msaaDepthRT = context.GetRenderTarget(ConstRenderPref::MSAA_Depth);

    // バリア設定
    std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
    RendererUtility::simple_change_target_state(barriers, msaaColorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    RendererUtility::simple_change_target_state(barriers, msaaDepthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    if (!barriers->empty())
    {
        cmdList->ResourceBarrier(barriers->size(), barriers->data());
    }

    msaaColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    msaaDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Clear
    const float clearColor[] = { 0.0, 0.0, 0.0, 1 };
    cmdList->ClearRenderTargetView(msaaColorRT->GetRTVHandle(), clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(msaaDepthRT->GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    auto sceneDepthRHandle = msaaDepthRT->GetDSVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[] = { msaaColorRT->GetRTVHandle() };

    // Viewport & Scissor (画面サイズ)
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

    cmdList->OMSetRenderTargets(1, sceneColorRTVHandle, FALSE, &sceneDepthRHandle);
}

// =========================================================
// Draw
// =========================================================
void GeometryPass::Draw(RenderContext& context)
{
    auto cmdList = context.CommandList;

    if (m_RenderQueue.empty()) {
        return;
    }

    UINT frameIndex = g_Engine->CurrentBackBufferIndex();

    // 定数バッファ (Light)
    const auto& lightingCB = g_Scene->GetLightingManager()->GetConstantBuffer();
    cmdList->SetGraphicsRootConstantBufferView(1, lightingCB->GetAddress());

    // View, Proj行列 (Main Camera) - 左手座標系に統一
    const auto view = DirectX::XMMatrixLookAtLH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(), context.Camera->GetUpward());
    const auto proj = DirectX::XMMatrixPerspectiveFovLH(context.Camera->GetFOV(), context.Camera->GetAspect(), 0.3f, 5000.0f);

    // Descriptor Heap
    auto materialHeap = g_Engine->GetDescriptorHeap()->GetHeap();
    cmdList->SetDescriptorHeaps(1, &materialHeap);

    // Shadow Map SRV
    auto shadowMapRT = context.GetRenderTarget(ConstRenderPref::ShadowMap);
    cmdList->SetGraphicsRootDescriptorTable(4, shadowMapRT->GetSRVHandle()->gpuHandle);

    // --- ライト行列の計算 (シングルパス) ---
    auto lightManager = g_Scene->GetLightingManager();
    // TODO: ライトがない場合のガード
    auto directionLight = lightManager->GetDirectionalLights()[0];

    XMFLOAT3 lightDirF = directionLight->Direction;
    XMVECTOR lightDir = XMVector3Normalize(XMVectorSet(lightDirF.x, lightDirF.y, lightDirF.z, 0.0f));

    XMMATRIX lightViewProj;
    CalculateLightViewProj_Geometry(lightDir, lightViewProj);

    // 定数バッファ用に転置
    lightViewProj = XMMatrixTranspose(lightViewProj);
    // ------------------------------------

    for (auto& obj : m_RenderQueue)
    {
        auto constantBuffer = obj->GetConstantBuffer(frameIndex);

        auto pTransform = constantBuffer->GetPtr<SharedStruct::Transform>();

        // 基本情報のセット
        pTransform->World = obj->GetTransform();
        pTransform->View = view;
        pTransform->Proj = proj;

        // CameraPos
        XMFLOAT3 camPosF = context.Camera->GetEyePosFloat3();
        pTransform->CameraPosition = camPosF;

        // シングルシャドウマップ用の行列をセット
        pTransform->LightViewProj = lightViewProj;

        // GPUセット
        cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetAddress());

        // 描画
        auto model = obj->FindComponent<MeshRenderer>()->model;
        auto origin_data = g_ModelLoader->GetModelOriginData(model->m_name);

        for (size_t i = 0; i < model->m_Meshes.size(); i++)
        {
            auto vbView = model->m_Meshes[i]->get_vertex_buffer()->View();
            auto ibView = model->m_Meshes[i]->get_index_buffer()->View();

            auto materialBuffer = model->m_Materials[i]->GetConstantBuffer();
            auto pMaterial = materialBuffer->GetPtr<DirectX::XMFLOAT4>();
            pMaterial[0] = model->m_Materials[i]->GetColor();
            cmdList->SetGraphicsRootConstantBufferView(2, materialBuffer->GetAddress());

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdList->IASetVertexBuffers(0, 1, &vbView);
            cmdList->IASetIndexBuffer(&ibView);

            cmdList->SetGraphicsRootDescriptorTable(3, model->m_Materials[i]->GetSrvHandle()->gpuHandle);

            cmdList->DrawIndexedInstanced(origin_data[i].Indeices.size(), 1, 0, 0, 0);
        }
    }
}