#pragma once

#include "GUI/Core/IDrawWindow.h"
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
    // paneHeightY > 0 のとき、Childの高さを指定（左カラム上下均等用）
    void DrawWorkListColumn(float paneHeightY = 0.0f);
    void DrawWorkFlowColumn(float paneHeightY = 0.0f);
    void DrawTaskColumn();

    // シーンから利用可能なTriggerComponentを収集するヘルパー
    void RefreshTriggerCache();

    // --- GUIの状態管理用変数 ---
    Work* m_selectedWork;
    WorkFlow* m_selectedWorkflow;
    
    // 新規作成用のバッファ
    char m_newWorkNameBuffer[128];
    char m_newFlowNameBuffer[128];

    // シーン内の全TriggerComponentのキャッシュ（「利用可能なタスク」リスト用）
    std::vector<TriggerComponent*> m_sceneTriggersCache;
};