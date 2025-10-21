#include "SceneManager.h"
#include "Scene/ISceneBase.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Scene/GameObject/Loader/GameObjectLoader.h"

// App.cppで実体を定義する
extern std::shared_ptr<ISceneBase> g_Scene;
extern Engine* g_Engine;

void SceneManager::LoadScene(const std::string& scenePath)
{
    // 次にロードするシーンのパスを保存し、リクエストフラグを立てる
    SetNextScenePath(scenePath);
    SetIsRequested(true);
}

void SceneManager::ProcessSceneRequest()
{
    // シーン切り替えのリクエストがなければ何もしない
    if (!m_isLoadRequested)
    {
        return;
    }

    // 古いシーンのリソースを解放する
    if (g_Scene)
    {
        // 描画が完全に終了するのを待つ
        g_Engine->WaitForGPU(); 
        g_Scene->shutdown();
    }

    // 新しいシーンを生成
    auto newScene = std::make_shared<DefaultScene>();
    
    // 4. 新しいシーンを初期化
    if (!newScene->Init(m_nextScenePath))
    {
        // 初期化失敗
        return;
    }

    // 5. グローバルなシーンポインタを新しいシーンに差し替える
    g_Scene = newScene;

    // 6. リクエストフラグを降ろす
    m_isLoadRequested = false;
    printf("Scene '%s' loaded successfully.\n", m_nextScenePath.c_str());
}
