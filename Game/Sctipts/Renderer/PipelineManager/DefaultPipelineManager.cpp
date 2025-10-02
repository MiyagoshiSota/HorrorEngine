#include "DefaultPipelineManager.h"

#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Target/DepthStencilTarget.h"

DefaultPipelineManager::DefaultPipelineManager()
{
    // Passを追加
    AddRenderPass(std::make_shared<SceneSetupPass>());
    AddRenderPass(std::make_shared<GeometryPass>());

    // ターゲットの生成
    m_sceneColor = std::make_shared<RenderTarget>();
    m_sceneDepth = std::make_shared<DepthStencilTarget>();
    m_sceneColor->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, g_Engine->AllocateRtvHandle(), g_Engine->GetSrvHeap()->GetHeap()->GetCPUDescriptorHandleForHeapStart());
    m_sceneDepth->Create(g_Engine->Device(), WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, g_Engine->GetDsvHeap(), g_Engine->GetSrvHeap()->GetHeap()->GetCPUDescriptorHandleForHeapStart());

};

void DefaultPipelineManager::Execute()
{
    // コンテキストを生成
    RenderContext context(g_Engine->CommandList(),g_Scene->GetSceneCamera(),g_Scene->GetSceneRenderer(),g_Scene->GetGameObjects(),WINDOW_WIDTH,WINDOW_HEIGHT);

    // レンダーターゲットを設定
    context.AddRenderTarget("SceneColor",m_sceneColor);
    context.AddRenderTarget("SceneDepth",m_sceneDepth);

    // Passを実行
    for (auto& pass : m_renderPasses)
    {
        pass->Execute(g_Engine->CommandList(), context);
    }
}
