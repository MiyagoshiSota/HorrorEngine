#include "DefaultPipelineManager.h"

#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Target/DepthStencilTarget.h"

#include "../Pass/MonochromePass.h"

DefaultPipelineManager::DefaultPipelineManager()
{
    // Passを追加
    AddRenderPass(std::make_shared<SceneSetupPass>());
    AddRenderPass(std::make_shared<GeometryPass>());
	AddRenderPass(std::make_shared<MonochromePass>());

    // ターゲットの生成
    m_sceneColor = std::make_shared<RenderTarget>();
    m_sceneDepth = std::make_shared<DepthStencilTarget>();
    m_sceneColor->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_Engine->AllocateRtvHandle(),g_Engine->GetSrvHeap()->Allocate());
    m_sceneDepth->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, g_Engine->GetDsvHeap(), g_Engine->GetSrvHeap()->Allocate());
};

void DefaultPipelineManager::Execute()
{
    // コンテキストを生成
    RenderContext context(g_Engine->CommandList(),g_Scene->get_scene_camera(),g_Scene->get_game_objects(),WINDOW_WIDTH,WINDOW_HEIGHT);

    // レンダーターゲットを設定
    context.AddRenderTarget("SceneColor",m_sceneColor);
    context.AddRenderTarget("SceneDepth",m_sceneDepth);

    // Passを実行
    for (auto& pass : m_renderPasses)
    {
        pass->Execute(g_Engine->CommandList(), context);
    }
}
