#pragma once

#include "GUI/IDrawWindow.h"
#include <vector>
#include <string>
#include <memory>

#include "Core/Components/Work/Work.h"

// 前方宣言
class Scene;
class GameObject;

class DrawWorkManagerWindow : public IDrawWindow
{
public:
    DrawWorkManagerWindow();
    ~DrawWorkManagerWindow() = default;

    // IDrawWindowから継承した描画関数
    void draw() override;

private:
    // UIをセクションごとに分割するためのヘルパー関数
    void DrawWorkListColumn();
    void DrawWorkFlowColumn();
    void DrawTaskColumn();
    void DrawTaskCreatorPanel(); // ユーザーリクエスト 3 のためのUI

    // シーンから利用可能なTriggerComponentを収集するヘルパー
    void RefreshTriggerCache();

    // --- GUIの状態管理用変数 ---
    Work* m_selectedWork;
    WorkFlow* m_selectedWorkflow;
    
    // 新規作成用のバッファ
    char m_newWorkNameBuffer[128];
    char m_newFlowNameBuffer[128];
    char m_newTaskNameBuffer[128];

    // シーン内の全TriggerComponentのキャッシュ（「利用可能なタスク」リスト用）
    std::vector<TriggerComponent*> m_sceneTriggersCache;
    
    // タスク作成UI用
    int m_selectedGameObjectIndex; // ターゲットGameObjectのインデックス
    std::string m_selectedConditionName; // 選択中のCondition名
    std::string m_selectedActionName;    // 選択中のAction(Reward)名
};