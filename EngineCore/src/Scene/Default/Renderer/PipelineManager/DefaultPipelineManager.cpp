#include "DefaultPipelineManager.h"

#include "Core/App.h"
#include "Modules/PublicConst/const_path_pref.h"
#include "Modules/PublicConst/const_render_pref.h"
#include "Renderer/Engine.h"
#include "Renderer/Pass/RenderProcess/Pass/DebugPass.h"
#include "Renderer/Pass/RenderProcess/Pass/GeometryPass.h"
#include "Renderer/Target/DepthStencilTarget.h"

DefaultPipelineManager::DefaultPipelineManager()
{
	// SimpleShadowMapPassの初期化
	m_simpleShadowMapPass = std::make_shared<SimpleShadowMapPass>();
	//m_simpleShadowMapPass = std::make_shared<CascadesShadowMapPass>();

	// ParticleSystemの初期化
	m_rainParticleSystem = std::make_shared<RainParticleSystem>();
	
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

    m_sceneColor->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM,1,1,1,0, g_Engine->AllocateRtvHandle(),g_Engine->GetDescriptorHeap()->Allocate(1));
	m_tmpColorA->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
    m_tmpColorB->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_msaaTarget->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 8, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_shadowDepth->Create(g_Engine->Device(), 2048, 2048, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
    m_sceneDepth->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	// HACK:Depthの数が決め打ちになってるのでPass内のカスケードの数と合わせる
	m_cascadedShadowDepth->Create(g_Engine->Device(), 2048, 2048, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 3, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(3));
    m_msaaDepth->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 8, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	// PostProcessManagerの初期化
	m_postProcessManager = std::make_shared<PostProcessManager>();
    m_postProcessManager->LoadPresets(const_path_pref::PostProcessPresetsPath);
	m_postProcessManager->Init();

	// 一時レンダーターゲットプールの生成
	m_tempRenderTargetPool = std::make_shared<TempRenderTargetPool>();
};

void DefaultPipelineManager::Execute()
{
    // コンテキストを生成
    RenderContext context(g_Engine->CommandList(),g_Scene->get_scene_camera(),g_Scene->get_game_objects(), m_sceneColor, m_sceneDepth, WINDOW_WIDTH,WINDOW_HEIGHT,g_Scene->get_pipeline_state_manager(),m_tempRenderTargetPool);

    // レンダーターゲットを設定
    context.AddRenderTarget(const_render_pref::SceneColor,m_sceneColor);
    context.AddRenderTarget(const_render_pref::SceneDepth,m_sceneDepth);
	context.AddRenderTarget(const_render_pref::TmpColorA, m_tmpColorA);
    context.AddRenderTarget(const_render_pref::TmpColorB, m_tmpColorB);
	context.AddRenderTarget(const_render_pref::MSAART, m_msaaTarget);
	context.AddRenderTarget(const_render_pref::MSAA_Depth, m_msaaDepth);
	context.AddRenderTarget(const_render_pref::ShadowMap,m_shadowDepth);
	context.AddRenderTarget(const_render_pref::CascadedShadowMap, m_cascadedShadowDepth);

    // Shadow
	m_simpleShadowMapPass->LastExecute(context);

    // Mesh
    for (auto& pass : m_sceneRenderPasses)
    {
        pass->Execute(context);
    }

	// Particle
	m_rainParticleSystem->Execute(context);

	// PostProcess
    m_postProcessManager->ExecutePasses(context);
}
