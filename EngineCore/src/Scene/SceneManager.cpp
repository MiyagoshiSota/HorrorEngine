#include "SceneManager.h"
#include "Scene/ISceneBase.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Scene/GameObject/Loader/GameObjectLoader.h"
#include "Core/Components/Work/WorkManager.h"

// App.cppで実体を定義する

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
        // Day(Scene) に紐づく Works を破棄する
        WorkManager::GetInstance().Clear();

        // コマンドリストが開いている場合、閉じて実行する
        g_Engine->CloseAndExecuteCommandList();
        
        // 描画が完全に終了するのを待つ
        g_Engine->WaitForGPU();
        g_Scene->Shutdown();
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
