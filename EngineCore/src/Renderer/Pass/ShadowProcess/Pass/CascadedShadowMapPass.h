#pragma once

#include "Core/App.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Model/Model.h"

using namespace DirectX;

class CascadesShadowMapPass : public SceneRenderPassBase
{
public:
    const std::string TARGET_NAME = ConstRenderPref::CascadedShadowMap;

    // カスケード設定
    static const int kCascadeCount = 3;
    // 分割距離 (近景10m, 中景50m, 遠景200m)
    const std::array<float, kCascadeCount> m_CascadeSplits = { 20.0f, 30.0f, 40.0f };
    // シャドウマップの解像度 (リソース作成時のサイズと合わせる)
    const float kShadowMapSize = 2048.0f;

    void Collect(RenderContext& context) override
    {
        auto cmdList = context.CommandList;

        // パイプラインステートの設定
        auto name = "Geometry_Default";
        auto PSOname = "CascadedShadowMap";

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
        if (m_RenderQueue.empty()) return;

        UINT frameIndex = g_Engine->CurrentBackBufferIndex();

        // ライト情報の取得
        auto lightManager = g_Scene->GetLightingManager();
        // TODO: 複数ライト対応や、ライトがない場合のガード処理
        auto directionLight = lightManager->GetDirectionalLights()[0];

        XMFLOAT3 lightDirF = directionLight->Direction;
        // ライトの方向ベクトルを正規化
        XMVECTOR lightDir = XMVector3Normalize(XMVectorSet(lightDirF.x, lightDirF.y, lightDirF.z, 0.0f));

        // DSVハンドルの準備 (スライス切り替え用)
        auto device = g_Engine->Device();
        UINT dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        auto shadowDepthRT = context.GetRenderTarget(TARGET_NAME);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(shadowDepthRT->GetDSVHandle());

        // カスケードごとのループ描画
        float nearPlane = 0.1f; // カメラのNear
        float previousSplitDist = nearPlane;

        for (int cascadeIdx = 0; cascadeIdx < kCascadeCount; ++cascadeIdx)
        {
            float splitDist = m_CascadeSplits[cascadeIdx];

            // このカスケード用の ViewProj 行列を計算
            XMMATRIX lightView, lightProj;
            CalculateLightViewProj(lightDir, previousSplitDist, splitDist, lightView, lightProj);

            // レンダーターゲット(DSV)を該当スライスに切り替え
            CD3DX12_CPU_DESCRIPTOR_HANDLE currentDSVHandle = dsvHandle;
            currentDSVHandle.Offset(cascadeIdx, dsvDescriptorSize);

            // 今のスライスをクリアする
            cmdList->ClearDepthStencilView(currentDSVHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

            // ターゲットセット
            cmdList->OMSetRenderTargets(0, nullptr, FALSE, &currentDSVHandle);

            // 描画コマンド発行
            DrawObjectsForCascade(cmdList, lightView, lightProj, frameIndex);

            // 次のSplitの開始地点へ
            previousSplitDist = splitDist;
        }

        // --- リソースバリアの復帰 (読み取りモードへ) ---
        std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriersPost = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
        RendererUtility::simple_change_target_state(
            barriersPost,
            shadowDepthRT,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        if (!barriersPost->empty())
        {
            cmdList->ResourceBarrier(barriersPost->size(), barriersPost->data());
        }
        shadowDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

private:
    // ヘルパー: 特定のカスケード用にオブジェクトを描画
    void DrawObjectsForCascade(ID3D12GraphicsCommandList* cmdList, CXMMATRIX view, CXMMATRIX proj, UINT frameIndex)
    {
        // 転置済みの行列を作成
        XMMATRIX tView = XMMatrixTranspose(view);
        XMMATRIX tProj = XMMatrixTranspose(proj);

        // シェーダ側でカメラ位置を使っている場合の対策として、ライトの仮位置を計算
        XMVECTOR det;
        XMMATRIX invView = XMMatrixInverse(&det, view);
        XMFLOAT3 lightPos;
        XMStoreFloat3(&lightPos, invView.r[3]);

        for (auto& obj : m_RenderQueue)
        {
            auto constantBuffer = obj->GetShadowConstantBuffer(frameIndex);

            // 定数バッファの更新
            // ※ShadowPass用のShaderは、通常のTransform構造体の View/Proj を使って描画します
            auto pTransform = constantBuffer->GetPtr<SharedStruct::Transform>();

            // Transformの設定
            pTransform->World = obj->GetTransform(); // Worldは内部でTranspose済みならそのままでOK
            pTransform->View = tView;                 // ここにライトのViewを入れる
            pTransform->Proj = tProj;                 // ここにライトのProjを入れる

            // 必要ならカメラ位置(ライト位置)も更新
            pTransform->CameraPosition = lightPos;

            // GPUにセット
            cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetAddress());

            // オブジェクトを描画
            auto model = obj->find_component<MeshRenderer>()->model;
            auto origin_data = g_ModelLoader->GetModelOriginData(model->name);

            for (size_t i = 0; i < model->m_Meshes.size(); i++)
            {
                auto vbView = model->m_Meshes[i]->get_vertex_buffer()->View();
                auto ibView = model->m_Meshes[i]->get_index_buffer()->View();

                cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                cmdList->IASetVertexBuffers(0, 1, &vbView);
                cmdList->IASetIndexBuffer(&ibView);

                // Draw
                cmdList->DrawIndexedInstanced(origin_data[i].Indeices.size(), 1, 0, 0, 0);
            }
        }
    }

    // ヘルパー: 視錐台のコーナーポイントを取得 (World Space)
    std::vector<XMVECTOR> GetFrustumCornersWorldSpace(const XMMATRIX& viewProj)
    {
        std::vector<XMVECTOR> corners;
        // NDC座標系の8頂点
        std::vector<XMVECTOR> ndcCorners = {
            XMVectorSet(-1.0f, -1.0f, 0.0f, 1.0f),
            XMVectorSet(-1.0f,  1.0f, 0.0f, 1.0f),
            XMVectorSet(1.0f,  1.0f, 0.0f, 1.0f),
            XMVectorSet(1.0f, -1.0f, 0.0f, 1.0f),
            XMVectorSet(-1.0f, -1.0f, 1.0f, 1.0f),
            XMVectorSet(-1.0f,  1.0f, 1.0f, 1.0f),
            XMVectorSet(1.0f,  1.0f, 1.0f, 1.0f),
            XMVectorSet(1.0f, -1.0f, 1.0f, 1.0f),
        };

        XMVECTOR det;
        XMMATRIX invViewProj = XMMatrixInverse(&det, viewProj);

        for (const auto& ndc : ndcCorners)
        {
            XMVECTOR worldPos = XMVector3Transform(ndc, invViewProj);
            // 透視除算 (wで割る)
            worldPos = XMVectorDivide(worldPos, XMVectorSplatW(worldPos));
            corners.push_back(worldPos);
        }
        return corners;
    }

    // ヘルパー: カスケード用の行列計算 (CSMの核心部分)
    void CalculateLightViewProj(
        XMVECTOR lightDir,
        float nearPlane,
        float farPlane,
        XMMATRIX& outView,
        XMMATRIX& outProj)
    {
        auto camera = g_Scene->GetSceneCamera();

        // 1. サブ視錐台を作るためのカメラ行列を計算
        float fov = camera->GetFOV();
        float aspect = camera->GetAspect();
        XMVECTOR eye = camera->GetEyePos();
        XMVECTOR target = camera->GetTargetPos();
        XMVECTOR up = camera->GetUpward();

        XMMATRIX camView = XMMatrixLookAtRH(eye, target, up);
        XMMATRIX camProj = XMMatrixPerspectiveFovRH(fov, aspect, nearPlane, farPlane);
        XMMATRIX camViewProj = XMMatrixMultiply(camView, camProj);

        // 2. サブ視錐台の8頂点(World)を取得
        std::vector<XMVECTOR> corners = GetFrustumCornersWorldSpace(camViewProj);

        // 3. 8頂点の「中心点」を計算
        XMVECTOR center = XMVectorSet(0, 0, 0, 0);
        for (const auto& v : corners)
        {
            center = XMVectorAdd(center, v);
        }
        center = XMVectorScale(center, 1.0f / 8.0f);

        // 4. ライトのView行列作成 (中心点を見るように配置)
        // 中心からライト方向にバックした位置にカメラを置く
        XMVECTOR lightPos = XMVectorSubtract(center, XMVectorScale(lightDir, 100.0f));
        outView = XMMatrixLookAtRH(lightPos, center, XMVectorSet(0, 1, 0, 0));

        // 5. 8頂点をライト空間(View)に変換し、Bounding Box (Min/Max) を求める
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();

        for (const auto& v : corners)
        {
            XMVECTOR lv = XMVector3Transform(v, outView);
            XMFLOAT3 fv;
            XMStoreFloat3(&fv, lv);

            minX = std::min(minX, fv.x); maxX = std::max(maxX, fv.x);
            minY = std::min(minY, fv.y); maxY = std::max(maxY, fv.y);
            minZ = std::min(minZ, fv.z); maxZ = std::max(maxZ, fv.z);
        }

        // --- Texel Snapping (チラつき防止処理) ---
        // 1テクセルがワールド空間でどれくらいの大きさかを計算
        float worldUnitsPerTexel = (maxX - minX) / kShadowMapSize;

        // Min座標をテクセルサイズの倍数に丸める (床関数 floor)
        minX = floor(minX / worldUnitsPerTexel) * worldUnitsPerTexel;
        minY = floor(minY / worldUnitsPerTexel) * worldUnitsPerTexel;

        // Max座標も同じ基準で丸める
        maxX = floor(maxX / worldUnitsPerTexel) * worldUnitsPerTexel;
        maxY = floor(maxY / worldUnitsPerTexel) * worldUnitsPerTexel;
        // ------------------------------------

        // 6. Ortho行列作成 (Bounding Boxに合わせる)
        // Z範囲(Near/Far)は、手前のオブジェクトも影を落とせるように余裕を持たせる
        //float zMult = 5.0f;
        //// minZ, maxZ はライト空間で負の値になることもあるため注意して拡張
        //if (minZ < 0) minZ *= zMult; else minZ /= zMult;
        //if (maxZ > 0) maxZ *= zMult; else maxZ /= zMult;
        maxZ += 1000.0f; // 手前側に1000m余裕を持たせる
        minZ -= 1000.0f; // 奥側にも1000m余裕を持たせる

        // RH (右手系) なので Near/Far の指定に注意
        // 通常 OrthoRH(w, h, n, f) ですが、ここではMinMax指定なので OffCenter を使用
        // Zは手前がプラス、奥がマイナスのケースなど系によりますが、
        // ここでは -maxZ (Near), -minZ (Far) として設定します (視線方向が-Zの場合)
        outProj = XMMatrixOrthographicOffCenterRH(minX, maxX, minY, maxY, -maxZ, -minZ);
    }
};