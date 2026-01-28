#pragma once

#include "Core/App.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Model/Model.h"

class SimpleShadowMapPass : public SceneRenderPassBase
{
public:
    const std::string TARGET_NAME = ConstRenderPref::ShadowMap;

    void Collect(RenderContext& context) override
    {
        auto cmdList = context.CommandList;

        // パイプラインステートの設定
        // 影生成専用のPSOとRootSignatureを使用します
        // ※ VertexShaderのみ、またはPixelShaderがNullの構成
        auto name = "Geometry_Default";
        auto PSOname = "ShadowMap";

        // ルートシグネチャを設定
        cmdList->SetGraphicsRootSignature(g_Scene->GetPipelineStateManager()->GetRootSignature(name)->Get());

        // PSOを設定
        cmdList->SetPipelineState(g_Scene->GetPipelineStateManager()->GetPipelineState(PSOname)->Get());

        // 描画対象のオブジェクトを収集
		// TODO: シャドウキャスターのみ収集するようにフィルタリング
		// TODO: Frustum Culling も考慮する
        m_RenderQueue.clear();
        for (auto& obj : context.GameObjects)
        {
            m_RenderQueue.push_back(obj);
        }

        // シャドウマップ用のDSVを取得
        auto shadowDepthRT = context.GetRenderTarget(TARGET_NAME);

        // バリアの設定 (PIXEL_SHADER_RESOURCE -> DEPTH_WRITE)
        std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
        RendererUtility::simple_change_target_state(barriers, shadowDepthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);

        if (!barriers->empty())
        {
            cmdList->ResourceBarrier(barriers->size(), barriers->data());
        }
        shadowDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

        // クリア処理
        cmdList->ClearDepthStencilView(shadowDepthRT->GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // レンダーターゲットセット (Color=Null, Depth=ShadowMap)
        // 影のみなのでカラーバッファは不要です
        auto dsvHandle = shadowDepthRT->GetDSVHandle();
        cmdList->OMSetRenderTargets(0, nullptr, FALSE, &dsvHandle);

        // ビューポートとシザーの設定
        auto resourceDesc = shadowDepthRT->GetResource()->GetDesc();
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
    }

    void Draw(RenderContext& context) override
    {
        auto cmdList = context.CommandList;

        if (m_RenderQueue.empty()) {
            return;
        }

        UINT frameIndex = g_Engine->CurrentBackBufferIndex();

        // --- ライト行列の計算 ---
		// 一旦Directional Lightのみ対応
        auto lightManager = g_Scene->GetLightingManager();
		// TODO: 複数ライト対応
		auto directionLight = lightManager->GetDirectionalLights()[0];

        DirectX::XMFLOAT3 lightDirFloat3 = directionLight->Direction;
        DirectX::XMVECTOR lightDir = DirectX::XMVector3Normalize(
            DirectX::XMVectorSet(lightDirFloat3.x, lightDirFloat3.y, lightDirFloat3.z, 0.0f));
        DirectX::XMVECTOR lightUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        DirectX::XMVECTOR targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        DirectX::XMVECTOR lightPos = DirectX::XMVectorSubtract(targetPos, DirectX::XMVectorScale(lightDir, 50.0f));

        // View行列 (LookAt)
        DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtRH(lightPos, targetPos, lightUp);

        // Projection行列 (Orthographic = 平行投影)
        // 太陽光の影はパースがつかないため、PerspectiveではなくOrthoを使います
        float sceneWidth = 20.0f; // 影を落とす範囲の広さ
        float sceneHeight = 20.0f;
        float nearZ = 1.0f;
        float farZ = 200.0f;
        DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicRH(sceneWidth, sceneHeight, nearZ, farZ);

        for (auto& obj : m_RenderQueue)
        {
            // 定数バッファの更新
            auto constantBuffer = obj->GetShadowConstantBuffer(frameIndex);
            auto pTransform = constantBuffer->GetPtr<SharedStruct::Transform>();

            // Transformの設定
            pTransform->World = obj->GetTransform(); // Worldは内部でTranspose済みならそのままでOK

            // ★ここを修正！ GPUに送る前に「転置(Transpose)」します
            pTransform->View = DirectX::XMMatrixTranspose(lightView);
            pTransform->Proj = DirectX::XMMatrixTranspose(lightProj);

            // LightViewProj も送るなら、掛けてから転置
            DirectX::XMMATRIX lightVP = DirectX::XMMatrixMultiply(lightView, lightProj);
            pTransform->LightViewProj = DirectX::XMMatrixTranspose(lightVP);

            // シェーダ側でカメラ位置を使っている場合の対策 (ライト位置を入れておく)
            DirectX::XMStoreFloat3(&pTransform->CameraPosition, lightPos);

            // GPUにセット
            cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetAddress());

            // オブジェクトを描画
            auto model = obj->FindComponent<MeshRenderer>()->model;
            auto origin_data = g_ModelLoader->GetModelOriginData(model->m_name);

            for (size_t i = 0; i < model->m_Meshes.size(); i++)
            {
                auto vbView = model->m_Meshes[i]->get_vertex_buffer()->View();
                auto ibView = model->m_Meshes[i]->get_index_buffer()->View();

                cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                cmdList->IASetVertexBuffers(0, 1, &vbView);
                cmdList->IASetIndexBuffer(&ibView);

                cmdList->DrawIndexedInstanced(origin_data[i].Indeices.size(), 1, 0, 0, 0);
            }
        }

        // --- リソースバリアの復帰 ---
        // 次の GeometryPass で SRV (テクスチャ) として読むために状態を遷移させる
        auto shadowDepthRT = context.GetRenderTarget(TARGET_NAME);

        std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriersPost = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();

        RendererUtility::simple_change_target_state(
            barriersPost,
            shadowDepthRT,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // または GENERIC_READ
        );

        if (!barriersPost->empty())
        {
            cmdList->ResourceBarrier(barriersPost->size(), barriersPost->data());
        }

        shadowDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

private:
};