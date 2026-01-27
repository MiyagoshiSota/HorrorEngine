#include "DefaultPipelineManager.h"

#include "Core/App.h"
#include "Modules/PublicConst/ConstPathPref.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Renderer/Engine.h"
#include "Renderer/Pass/RenderProcess/Pass/DebugPass.h"
#include "Renderer/Pass/RenderProcess/Pass/GeometryPass.h"
#include "Scene/Skybox/SkyboxManager.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Renderer/Target/DepthStencilTarget.h"
#include <d3dx12.h>

DefaultPipelineManager::DefaultPipelineManager()
{
	// SimpleShadowMapPassの初期化
	m_simpleShadowMapPass = std::make_shared<SimpleShadowMapPass>();
	//m_simpleShadowMapPass = std::make_shared<CascadesShadowMapPass>();

	// ParticleSystemの初期化
	m_rainParticleSystem = std::make_shared<RainParticleSystem>();
	
	// SkyboxPassの初期化
	m_skyboxPass = std::make_shared<SkyboxPass>();

    // Passを追加
    AddRenderProcessPass(std::make_shared<GeometryPass>());
	// AddRenderProcessPass(std::make_shared<DebugPass>());

    // ターゲットの生成
    m_sceneColor = std::make_shared<RenderTarget>();
	m_tmpColorA = std::make_shared<RenderTarget>();
	m_tmpColorB = std::make_shared<RenderTarget>();
	m_msaaTarget = std::make_shared<RenderTarget>();
	m_shadowDepth = std::make_shared<DepthStencilTarget>();
	m_cascadedShadowDepth = std::make_shared<DepthStencilTarget>();
	m_msaaDepth = std::make_shared<DepthStencilTarget>();
    m_sceneDepth = std::make_shared<DepthStencilTarget>();

    m_sceneColor->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM,1,1,1,0, g_Engine->AllocateRtvHandle(),g_Engine->GetDescriptorHeap()->Allocate(1));
	m_tmpColorA->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
    m_tmpColorB->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_msaaTarget->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 8, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_shadowDepth->Create(g_Engine->Device(), 2048, 2048, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
    m_sceneDepth->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	// HACK:Depthの数が決め打ちになってるのでPass内のカスケードの数と合わせる
	m_cascadedShadowDepth->Create(g_Engine->Device(), 2048, 2048, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 3, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(3));
    m_msaaDepth->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 8, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	// PostProcessManagerの初期化
	m_postProcessManager = std::make_shared<PostProcessManager>();
    m_postProcessManager->LoadPresets(ConstPathPref::kPostProcessPresetsPath);
	m_postProcessManager->Init();

	// 一時レンダーターゲットプールの生成
	m_tempRenderTargetPool = std::make_shared<TempRenderTargetPool>();
}

void DefaultPipelineManager::Execute()
{
    // コンテキストを生成
    RenderContext context(g_Engine->CommandList(),g_Scene->GetSceneCamera(),g_Scene->GetGameObjects(), m_sceneColor, m_sceneDepth, kWindowWidth,kWindowHeight,g_Scene->GetPipelineStateManager(),m_tempRenderTargetPool);

    // レンダーターゲットを設定
    context.AddRenderTarget(ConstRenderPref::SceneColor,m_sceneColor);
    context.AddRenderTarget(ConstRenderPref::SceneDepth,m_sceneDepth);
	context.AddRenderTarget(ConstRenderPref::TmpColorA, m_tmpColorA);
    context.AddRenderTarget(ConstRenderPref::TmpColorB, m_tmpColorB);
	context.AddRenderTarget(ConstRenderPref::MSAART, m_msaaTarget);
	context.AddRenderTarget(ConstRenderPref::MSAA_Depth, m_msaaDepth);
	context.AddRenderTarget(ConstRenderPref::ShadowMap,m_shadowDepth);
	context.AddRenderTarget(ConstRenderPref::CascadedShadowMap, m_cascadedShadowDepth);

    // Skyboxデータを設定
    auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
    if (defaultScene)
    {
        auto skyboxManager = defaultScene->GetSkyboxManager();
        if (skyboxManager && skyboxManager->IsValid())
        {
            auto renderData = skyboxManager->GetRenderData();
            RenderContext::SkyboxData skyboxData;
            skyboxData.vertexBuffer = renderData.vertexBuffer;
            skyboxData.indexBuffer = renderData.indexBuffer;
            skyboxData.indexCount = renderData.indexCount;
            skyboxData.constantBuffer = renderData.constantBuffer;
            skyboxData.srvHandle = renderData.srvHandle;
            skyboxData.isValid = true;
            context.SetSkyboxData(skyboxData);

            // 定数バッファ更新用のコールバックを設定
            context.SetSkyboxUpdateCallback([skyboxManager](DirectX::XMMATRIX viewProj) {
                skyboxManager->UpdateConstantBuffer(viewProj);
            });
        }
    }

    // Shadow
	m_simpleShadowMapPass->LastExecute(context);

    // Mesh
    for (auto& pass : m_sceneRenderPasses)
    {
        pass->Execute(context);
    }

    // Skybox (MSAAターゲットに描画、GeometryPassの後)
    if (m_skyboxPass && m_skyboxPass->IsEnabled(context))
    {
        m_skyboxPass->Execute(context);
    }

    // MSAA Resolve処理
    RendererUtility::ResolveMSAA(context, ConstRenderPref::MSAART, ConstRenderPref::SceneColor);

	// Particle
	m_rainParticleSystem->Execute(context);

	// PostProcess
    m_postProcessManager->ExecutePasses(context);
}
