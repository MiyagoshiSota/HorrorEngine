#pragma once
#include <string>
#include <memory>

#include "Modules/PublicConst/ConstPathPref.h"

class ISceneBase; // 前方宣言

class SceneManager
{
public:
    SceneManager()
    {
		m_isLoadRequested = false;
		m_nextScenePath = ConstPathPref::kDefaultGameObjectPath;
    }

    // シーンの読み込みを予約する
    void LoadScene(const std::string& scenePath);

    // 毎フレームの開始時に呼び出し、予約があればシーンを切り替える
    void ProcessSceneRequest();

    // シーン読み込みが予約されているか取得
    std::string GetNextScenePath() const { return m_nextScenePath; }
    
    // 現在のシーンパスを更新する（シーンを再読み込みせずにパスのみ更新）
    void UpdateCurrentScenePath(const std::string& scenePath) { SetNextScenePath(scenePath); }
    
private:
    void SetNextScenePath(std::string scenePath) { m_nextScenePath = scenePath; }
    void SetIsRequested(bool isRequested) { m_isLoadRequested = isRequested; }
    
private:
    bool m_isLoadRequested = false;
    std::string m_nextScenePath;
};

// グローバルなSceneManagerインスタンス
extern std::unique_ptr<SceneManager> g_SceneManager;
