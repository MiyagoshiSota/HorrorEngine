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
#include "Scene/Default/Scene/DefaultScene.h"
#include "Scene/Default/Renderer/PipelineManager/DefaultPipelineManager.h"

using namespace DirectX;

// --- 定数定義 ---
static const float kShadowMapSize = 2048.0f;
static const float kShadowDistance = 10000.0f;

// SimpleShadowMapPass と同一のライト View*Proj を使用すること（posLight とシャドウマップの座標系一致のため）
static const float kShadowSceneWidth = 50.0f;
static const float kShadowSceneHeight = 50.0f;
// static const float kShadowNearZ = 1.0f;
static const float kShadowNearZ = 0.0f;
// static const float kShadowFarZ = 150.0f;
static const float kShadowFarZ = 10.0f;
static const float kShadowLightDistance = 25.0f;

void CalculateLightViewProj_Geometry(
    XMVECTOR lightDir,
    XMMATRIX& outViewProj)
{
    XMVECTOR targetPos = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR lightPos = XMVectorSubtract(targetPos, XMVectorScale(lightDir, kShadowLightDistance));
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX lightView = XMMatrixLookAtRH(lightPos, targetPos, up);
    XMMATRIX lightProj = XMMatrixOrthographicRH(kShadowSceneWidth, kShadowSceneHeight, kShadowNearZ, kShadowFarZ);

    outViewProj = XMMatrixMultiply(lightView, lightProj);
}

// =========================================================
// Collect
// =========================================================
void GeometryPass::Collect(RenderContext& context)
{
    auto cmdList = context.CommandList;

    m_RenderQueue.clear();
    for (auto& obj : context.GameObjects)
    {
        m_RenderQueue.push_back(obj);
    }

    m_useDeferred = (context.GetRenderTarget(ConstRenderPref::GBufferAlbedo) != nullptr);

    if (m_useDeferred)
    {
        // デファード: Geometry_GBuffer + GBufferPass（6 RTV）。LightingPass でライティング・影
        const char* name = "Geometry_GBuffer";
        const char* PSOname = "GBufferPass";

        cmdList->SetGraphicsRootSignature(g_Scene->GetPipelineStateManager()->GetRootSignature(name)->Get());
        cmdList->SetPipelineState(g_Scene->GetPipelineStateManager()->GetPipelineState(PSOname)->Get());

        std::shared_ptr<ITargetBase> colorRT = context.GetRenderTarget(ConstRenderPref::GBufferAlbedo);
        std::shared_ptr<ITargetBase> depthRT = context.GetRenderTarget(ConstRenderPref::SceneDepth);
        if (!colorRT || !depthRT)
            return;

        auto motionVectorRT = context.GetRenderTarget(ConstRenderPref::MotionVectorBuffer);
        auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
        auto worldPositionRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);
        auto materialRT = context.GetRenderTarget(ConstRenderPref::GBufferMaterial);
        auto emissiveRT = context.GetRenderTarget(ConstRenderPref::GBufferEmissive);

        std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
        RendererUtility::simple_change_target_state(barriers, colorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        RendererUtility::simple_change_target_state(barriers, depthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        if (motionVectorRT) RendererUtility::simple_change_target_state(barriers, motionVectorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (normalRT) RendererUtility::simple_change_target_state(barriers, normalRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (worldPositionRT) RendererUtility::simple_change_target_state(barriers, worldPositionRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (materialRT) RendererUtility::simple_change_target_state(barriers, materialRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (emissiveRT) RendererUtility::simple_change_target_state(barriers, emissiveRT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        if (!barriers->empty())
            cmdList->ResourceBarrier(barriers->size(), barriers->data());

        colorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        depthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
        if (motionVectorRT) motionVectorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (normalRT) normalRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (worldPositionRT) worldPositionRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (materialRT) materialRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        if (emissiveRT) emissiveRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

        const float clearColor[] = { 0.0, 0.0, 0.0, 1 };
        const float clearMotionVector[] = { 0.0, 0.0, 0.0, 0.0 };
        const float clearNormal[] = { 0.0, 0.0, 1.0, 1.0 };
        const float clearWorldPos[] = { 0.0, 0.0, 0.0, 1.0 };
        const float clearMaterial[] = { 0.5, 0.0, 1.0, 0.0 };
        const float clearEmissive[] = { 0.0, 0.0, 0.0, 1.0 };
        cmdList->ClearRenderTargetView(colorRT->GetRTVHandle(), clearColor, 0, nullptr);
        if (motionVectorRT) cmdList->ClearRenderTargetView(motionVectorRT->GetRTVHandle(), clearMotionVector, 0, nullptr);
        if (normalRT) cmdList->ClearRenderTargetView(normalRT->GetRTVHandle(), clearNormal, 0, nullptr);
        if (worldPositionRT) cmdList->ClearRenderTargetView(worldPositionRT->GetRTVHandle(), clearWorldPos, 0, nullptr);
        if (materialRT) cmdList->ClearRenderTargetView(materialRT->GetRTVHandle(), clearMaterial, 0, nullptr);
        if (emissiveRT) cmdList->ClearRenderTargetView(emissiveRT->GetRTVHandle(), clearEmissive, 0, nullptr);
        cmdList->ClearDepthStencilView(depthRT->GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr); // Reversed-Z

        D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[6] = {
            colorRT->GetRTVHandle(),
            motionVectorRT ? motionVectorRT->GetRTVHandle() : D3D12_CPU_DESCRIPTOR_HANDLE{},
            normalRT ? normalRT->GetRTVHandle() : D3D12_CPU_DESCRIPTOR_HANDLE{},
            worldPositionRT ? worldPositionRT->GetRTVHandle() : D3D12_CPU_DESCRIPTOR_HANDLE{},
            materialRT ? materialRT->GetRTVHandle() : D3D12_CPU_DESCRIPTOR_HANDLE{},
            emissiveRT ? emissiveRT->GetRTVHandle() : D3D12_CPU_DESCRIPTOR_HANDLE{}
        };
        D3D12_CPU_DESCRIPTOR_HANDLE sceneDepthRHandle = depthRT->GetDSVHandle();
        cmdList->OMSetRenderTargets(6u, sceneColorRTVHandle, FALSE, &sceneDepthRHandle);
    }
    else
    {
        // フォワード: Geometry_Default + DefaultPipelinePass（4 RTV）。SimplePS でライティング・影を一括
        bool msaaEnabled = true;
        auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
        if (defaultScene)
        {
            auto pipeline = defaultScene->GetDefaultPipelineManager();
            if (pipeline)
                msaaEnabled = pipeline->GetAASettings().msaaEnabled;
        }

        const char* name = "Geometry_Default";
        const char* PSOname = msaaEnabled ? "DefaultPipelinePass" : "DefaultPipelinePassNoMSAA";

        cmdList->SetGraphicsRootSignature(g_Scene->GetPipelineStateManager()->GetRootSignature(name)->Get());
        cmdList->SetPipelineState(g_Scene->GetPipelineStateManager()->GetPipelineState(PSOname)->Get());

        std::shared_ptr<ITargetBase> colorRT = msaaEnabled
            ? context.GetRenderTarget(ConstRenderPref::MSAART)
            : context.GetRenderTarget(ConstRenderPref::SceneColor);
        std::shared_ptr<ITargetBase> depthRT = msaaEnabled
            ? context.GetRenderTarget(ConstRenderPref::MSAA_Depth)
            : context.GetRenderTarget(ConstRenderPref::SceneDepth);
        auto motionVectorRT = context.GetRenderTarget(ConstRenderPref::MotionVectorBuffer);
        auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
        auto worldPositionRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);

        if (!colorRT || !depthRT || !motionVectorRT || !normalRT || !worldPositionRT)
            return;

        std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
        RendererUtility::simple_change_target_state(barriers, colorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        RendererUtility::simple_change_target_state(barriers, depthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        RendererUtility::simple_change_target_state(barriers, motionVectorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        RendererUtility::simple_change_target_state(barriers, normalRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        RendererUtility::simple_change_target_state(barriers, worldPositionRT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        if (!barriers->empty())
            cmdList->ResourceBarrier(barriers->size(), barriers->data());

        colorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        depthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
        motionVectorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        normalRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
        worldPositionRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);

        const float clearColor[] = { 0.0, 0.0, 0.0, 1 };
        const float clearMotionVector[] = { 0.0, 0.0, 0.0, 0.0 };
        const float clearNormal[] = { 0.0, 0.0, 1.0, 1.0 };
        const float clearWorldPos[] = { 0.0, 0.0, 0.0, 1.0 };
        cmdList->ClearRenderTargetView(colorRT->GetRTVHandle(), clearColor, 0, nullptr);
        cmdList->ClearRenderTargetView(motionVectorRT->GetRTVHandle(), clearMotionVector, 0, nullptr);
        cmdList->ClearRenderTargetView(normalRT->GetRTVHandle(), clearNormal, 0, nullptr);
        cmdList->ClearRenderTargetView(worldPositionRT->GetRTVHandle(), clearWorldPos, 0, nullptr);
        cmdList->ClearDepthStencilView(depthRT->GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr); // Reversed-Z

        D3D12_CPU_DESCRIPTOR_HANDLE forwardRTVHandle[4] = {
            colorRT->GetRTVHandle(),
            motionVectorRT->GetRTVHandle(),
            normalRT->GetRTVHandle(),
            worldPositionRT->GetRTVHandle()
        };
        D3D12_CPU_DESCRIPTOR_HANDLE forwardDepthHandle = depthRT->GetDSVHandle();
        cmdList->OMSetRenderTargets(4u, forwardRTVHandle, FALSE, &forwardDepthHandle);
    }

    // Viewport & Scissor（共通）
    std::shared_ptr<ITargetBase> anyRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
    if (!anyRT)
        anyRT = context.GetRenderTarget(ConstRenderPref::MSAART);
    if (!anyRT)
        anyRT = context.GetRenderTarget(ConstRenderPref::GBufferAlbedo);
    if (anyRT && anyRT->GetResource())
    {
        auto resourceDesc = anyRT->GetResource()->GetDesc();
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(resourceDesc.Width);
        viewport.Height = static_cast<float>(resourceDesc.Height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        cmdList->RSSetViewports(1, &viewport);
        D3D12_RECT scissorRect = {};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = static_cast<LONG>(resourceDesc.Width);
        scissorRect.bottom = static_cast<LONG>(resourceDesc.Height);
        cmdList->RSSetScissorRects(1, &scissorRect);
    }
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
    // TAA有効時は、RenderContext経由でジッター適用済みの投影行列を取得
    const auto view = DirectX::XMMatrixLookAtLH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(), context.Camera->GetUpward());
    const auto proj = context.GetProjectionMatrix();
    
    // モーションベクター用：ジッターなしの投影行列を取得
    // （モーションベクターにはカメラのジッター差分を含めたくないため）
    const auto projNoJitter = context.GetNonJitteredProjectionMatrix();

    // Descriptor Heap
    auto materialHeap = g_Engine->GetDescriptorHeap()->GetHeap();
    cmdList->SetDescriptorHeaps(1, &materialHeap);

    // フォワード時のみシャドウマップをバインド（Geometry_Default の table t4）。デファード時は LightingPass で参照
    if (!m_useDeferred)
    {
        auto shadowRT = context.GetRenderTarget(ConstRenderPref::ShadowMap);
        if (shadowRT && shadowRT->GetSRVHandle())
            cmdList->SetGraphicsRootDescriptorTable(4, shadowRT->GetSRVHandle()->gpuHandle);
    }

    // --- ライト行列の計算 (VS の posLight 出力用・LightingPass/SimplePS で使用) ---
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

    // 現フレームのViewProj行列を計算（モーションベクター用、ジッターなし）
    XMMATRIX currViewProjNoJitter = XMMatrixMultiply(view, projNoJitter);
    
    // 最初のフレームは前フレーム行列を現フレームと同じにする
    if (m_isFirstFrame)
    {
        m_prevViewProj = currViewProjNoJitter;
        m_isFirstFrame = false;
    }

    for (auto& obj : m_RenderQueue)
    {
        auto constantBuffer = obj->GetConstantBuffer(frameIndex);

        auto pTransform = constantBuffer->GetPtr<SharedStruct::Transform>();

        // 基本情報のセット
        pTransform->World = obj->GetTransform();
        pTransform->View = view;
        pTransform->Proj = proj;
        pTransform->UseRayTracedShadow = m_useDeferred ? 0 : (context.UseRayTracedShadow() ? 1 : 0);
        pTransform->InvRayTracedShadowMapSize = context.GetInvRayTracedShadowMapSize();

        // CameraPos
        XMFLOAT3 camPosF = context.Camera->GetEyePosFloat3();
        pTransform->CameraPosition = camPosF;

        // シングルシャドウマップ用の行列をセット
        pTransform->LightViewProj = lightViewProj;
        
        // Motion Vector用：ViewProj行列をセット
        pTransform->PrevViewProj = XMMatrixTranspose(m_prevViewProj);
        pTransform->CurrViewProj = XMMatrixTranspose(currViewProjNoJitter);

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
    
    // 現フレームのViewProj行列（ジッターなし）を次フレーム用に保存
    m_prevViewProj = currViewProjNoJitter;
}