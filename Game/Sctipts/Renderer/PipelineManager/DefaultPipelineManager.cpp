#include "DefaultPipelineManager.h"

#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Pass/RenderProcess/Pass/GeometryPass.h"
#include "Renderer/Target/DepthStencilTarget.h"

DefaultPipelineManager::DefaultPipelineManager()
{
    // Passを追加
    AddRenderProcessPass(std::make_shared<GeometryPass>());

    // ターゲットの生成
    m_sceneColor = std::make_shared<RenderTarget>();
	m_tmpColorA = std::make_shared<RenderTarget>();
	m_tmpColorB = std::make_shared<RenderTarget>();
    m_sceneDepth = std::make_shared<DepthStencilTarget>();

    m_sceneColor->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_Engine->AllocateRtvHandle(),g_Engine->GetSrvHeap()->Allocate());
	m_tmpColorA->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_Engine->AllocateRtvHandle(), g_Engine->GetSrvHeap()->Allocate());
    m_tmpColorB->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_Engine->AllocateRtvHandle(), g_Engine->GetSrvHeap()->Allocate());
    m_sceneDepth->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, g_Engine->GetDsvHeap(), g_Engine->GetSrvHeap()->Allocate());

	m_postProcessManager = std::make_shared<PostProcessManager>();
};

void DefaultPipelineManager::Execute()
{
    // コンテキストを生成
    RenderContext context(g_Engine->CommandList(),g_Scene->get_scene_camera(),g_Scene->get_game_objects(), m_sceneColor, m_sceneDepth, WINDOW_WIDTH,WINDOW_HEIGHT);

    // レンダーターゲットを設定
    context.AddRenderTarget("SceneColor",m_sceneColor);
    context.AddRenderTarget("SceneDepth",m_sceneDepth);
	context.AddRenderTarget("TmpColorA", m_tmpColorA);
    context.AddRenderTarget("TmpColorB", m_tmpColorB);

    // RenderPassを実行
    for (auto& pass : m_sceneRenderPasses)
    {
        pass->Execute(context);
    }

    m_postProcessManager->ExecutePasses(context);
}
