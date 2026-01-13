#include "GeometryPass.h"

#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <cmath>

#include "Core/App.h"
#include "Modules/PublicConst/const_render_pref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Model/Model.h"

using namespace DirectX;

// --- 定数定義 ---
static const int CASCADE_COUNT = 3;
static const float SHADOW_MAP_SIZE = 2048.0f;
static const std::array<float, CASCADE_COUNT> CASCADE_SPLITS = { 10.0f, 50.0f, 200.0f };

// --- ヘルパー関数: CSM用行列計算 ---
// (CascadesShadowMapPassと同じロジック)
void CalculateLightViewProj_Geometry(
    XMVECTOR lightDir,
    float nearPlane,
    float farPlane,
    XMMATRIX& outView,
    XMMATRIX& outProj)
{
	auto camera = g_Scene->get_scene_camera();
    float fov = camera->GetFOV();
    float aspect = camera->GetAspect();
    XMVECTOR eye = camera->GetEyePos();
    XMVECTOR target = camera->GetTargetPos();
    XMVECTOR up = camera->GetUpward();

    XMMATRIX camView = XMMatrixLookAtRH(eye, target, up);
    XMMATRIX camProj = XMMatrixPerspectiveFovRH(fov, aspect, nearPlane, farPlane);
    XMMATRIX camViewProj = XMMatrixMultiply(camView, camProj);

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
    XMMATRIX invViewProj = XMMatrixInverse(&det, camViewProj);
    std::vector<XMVECTOR> corners;
    for (const auto& ndc : ndcCorners) {
        XMVECTOR worldPos = XMVector3Transform(ndc, invViewProj);
        worldPos = XMVectorDivide(worldPos, XMVectorSplatW(worldPos));
        corners.push_back(worldPos);
    }

    XMVECTOR center = XMVectorSet(0, 0, 0, 0);
    for (const auto& v : corners) center = XMVectorAdd(center, v);
    center = XMVectorScale(center, 1.0f / 8.0f);

    XMVECTOR lightPos = XMVectorSubtract(center, XMVectorScale(lightDir, 100.0f));
    outView = XMMatrixLookAtRH(lightPos, center, XMVectorSet(0, 1, 0, 0));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (const auto& v : corners) {
        XMVECTOR lv = XMVector3Transform(v, outView);
        XMFLOAT3 fv;
        XMStoreFloat3(&fv, lv);
        minX = std::min(minX, fv.x); maxX = std::max(maxX, fv.x);
        minY = std::min(minY, fv.y); maxY = std::max(maxY, fv.y);
        minZ = std::min(minZ, fv.z); maxZ = std::max(maxZ, fv.z);
    }

    float worldUnitsPerTexel = (maxX - minX) / SHADOW_MAP_SIZE;
    minX = floor(minX / worldUnitsPerTexel) * worldUnitsPerTexel;
    minY = floor(minY / worldUnitsPerTexel) * worldUnitsPerTexel;
    maxX = floor(maxX / worldUnitsPerTexel) * worldUnitsPerTexel;
    maxY = floor(maxY / worldUnitsPerTexel) * worldUnitsPerTexel;

    float zMult = 10.0f;
    if (minZ < 0) minZ *= zMult; else minZ /= zMult;
    if (maxZ > 0) maxZ *= zMult; else maxZ /= zMult;
    outProj = XMMatrixOrthographicOffCenterRH(minX, maxX, minY, maxY, -maxZ, -minZ);
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

    cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(name)->get());
    // ※注意: ここのPSOは MSAA Count=8 に設定されている必要があります
    cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(PSOname)->Get());

    m_RenderQueue.clear();
    for (auto& obj : context.GameObjects)
    {
    	m_RenderQueue.push_back(obj);
    }

    auto msaaColorRT = context.GetRenderTarget(const_render_pref::MSAART);
    auto msaaDepthRT = context.GetRenderTarget(const_render_pref::MSAA_Depth);

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
    const auto& lightingCB = g_Scene->get_lighting_manager()->get_constant_buffer();
    cmdList->SetGraphicsRootConstantBufferView(1, lightingCB->GetAddress());

    // View, Proj行列 (Main Camera)
    const auto view = DirectX::XMMatrixLookAtRH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(), context.Camera->GetUpward());
    const auto proj = DirectX::XMMatrixPerspectiveFovRH(context.Camera->GetFOV(), context.Camera->GetAspect(), 0.3f, 5000.0f);

    // Descriptor Heap
    auto materialHeap = g_Engine->GetDescriptorHeap()->GetHeap();
    cmdList->SetDescriptorHeaps(1, &materialHeap);

    // Shadow Map SRV
    auto shadowMapRT = context.GetRenderTarget(const_render_pref::CascadedShadowMap);
    cmdList->SetGraphicsRootDescriptorTable(4, shadowMapRT->GetSRVHandle()->gpuHandle);

    // --- CSM用行列計算 (CascadesShadowMapPassと共通) ---
    auto lightManager = g_Scene->get_lighting_manager();
    auto directionLight = lightManager->get_directional_lights()[0];

    XMFLOAT3 lightDirF = directionLight->Direction;
    XMVECTOR lightDir = XMVector3Normalize(XMVectorSet(lightDirF.x, lightDirF.y, lightDirF.z, 0.0f));

    XMMATRIX lightViewProjs[CASCADE_COUNT];
    float nearPlane = 0.1f;
    float previousSplitDist = nearPlane;

    for (int i = 0; i < CASCADE_COUNT; ++i)
    {
        float splitDist = CASCADE_SPLITS[i];
        XMMATRIX lView, lProj;

        CalculateLightViewProj_Geometry(lightDir, previousSplitDist, splitDist, lView, lProj);

        // ここで結合＆転置しておく
        lightViewProjs[i] = XMMatrixTranspose(XMMatrixMultiply(lView, lProj));

        previousSplitDist = splitDist;
    }
    // --------------------------------------------------

    for (auto& obj : m_RenderQueue)
    {
        auto constantBuffer = obj->get_constant_buffer(frameIndex);

        // ★重要: CascadedShadowMapTransform として取得
        auto pTransform = constantBuffer->GetPtr<SharedStruct::CascadedShadowMapTransform>();

        // 基本情報のセット
        pTransform->World = obj->get_transform();
        pTransform->View = view;
        pTransform->Proj = proj;

        // CameraPos (XMFLOAT3 -> XMVECTOR)
        XMFLOAT3 camPosF = context.Camera->GetEyePosFloat3();
        pTransform->CameraPosition = camPosF;

        // CSM情報のセット
        for (int i = 0; i < CASCADE_COUNT; ++i) {
            pTransform->LightViewProj[i] = lightViewProjs[i];
        }

        // SplitDepths (XMVECTOR)
        pTransform->SplitDepths = XMVectorSet(CASCADE_SPLITS[0], CASCADE_SPLITS[1], CASCADE_SPLITS[2], 0.0f);

        pTransform->NumCascades = CASCADE_COUNT;

        // GPUセット
        cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetAddress());

        // 描画
        auto model = obj->find_component<MeshRenderer>()->model;
        auto origin_data = g_ModelLoader->GetModelOriginData(model->name);

        for (size_t i = 0; i < model->m_Meshes.size(); i++)
        {
            auto vbView = model->m_Meshes[i]->get_vertex_buffer()->View();
            auto ibView = model->m_Meshes[i]->get_index_buffer()->View();

            auto materialBuffer = model->m_Materials[i]->get_constant_buffer();
            auto pMaterial = materialBuffer->GetPtr<DirectX::XMFLOAT4>();
            pMaterial[0] = model->m_Materials[i]->get_color();
            cmdList->SetGraphicsRootConstantBufferView(2, materialBuffer->GetAddress());

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdList->IASetVertexBuffers(0, 1, &vbView);
            cmdList->IASetIndexBuffer(&ibView);

            cmdList->SetGraphicsRootDescriptorTable(3, model->m_Materials[i]->get_srv_handle()->gpuHandle);

            cmdList->DrawIndexedInstanced(origin_data[i].Indeices.size(), 1, 0, 0, 0);
        }
    }

    // MSAA Resolve処理
    auto msaaColorRT = context.GetRenderTarget(const_render_pref::MSAART);
    auto sceneColorRT = context.GetRenderTarget("SceneColor");

    // Barrier: SceneColor -> ResolveDest
    if (sceneColorRT->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            sceneColorRT->GetResource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        cmdList->ResourceBarrier(1, &barrier);
    }

    D3D12_RESOURCE_BARRIER resolveBarriers[2];
    resolveBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
        msaaColorRT->GetResource().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE
    );
    resolveBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
        sceneColorRT->GetResource().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_RESOLVE_DEST
    );
    cmdList->ResourceBarrier(2, resolveBarriers);

    cmdList->ResolveSubresource(
        sceneColorRT->GetResource().Get(), 0,
        msaaColorRT->GetResource().Get(), 0,
        sceneColorRT->GetResource()->GetDesc().Format
    );

    // Barrier Restore
    std::vector<D3D12_RESOURCE_BARRIER> barriersPost;
    barriersPost.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        msaaColorRT->GetResource().Get(),
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));
    barriersPost.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        sceneColorRT->GetResource().Get(),
        D3D12_RESOURCE_STATE_RESOLVE_DEST,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    ));
    cmdList->ResourceBarrier(barriersPost.size(), barriersPost.data());

    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    msaaColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
}