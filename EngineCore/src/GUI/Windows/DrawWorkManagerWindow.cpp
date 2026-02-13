#include "DrawWorkManagerWindow.h"
#include "imgui.h"
#include "Core/Components/TriggerFactory.h" // TriggerFactoryシングルトン
#include "Scene/GameObject/GameObject.h"
#include <algorithm> // std::find

#include "Core/App.h"
#include "Core/Components/Work/WorkManager.h"
#include "Scene/SceneManager.h"

// コンストラクタ
DrawWorkManagerWindow::DrawWorkManagerWindow()
    : m_selectedWork(nullptr), m_selectedWorkflow(nullptr)
{
    memset(m_newWorkNameBuffer, 0, sizeof(m_newWorkNameBuffer));
    memset(m_newFlowNameBuffer, 0, sizeof(m_newFlowNameBuffer));
}

// 毎フレーム呼ばれるメインの描画関数
void DrawWorkManagerWindow::draw()
{
    // HACK:全部のTriggerを毎フレーム取得しているが、パフォーマンス的に問題があれば改善する
    RefreshTriggerCache();

    // 最小サイズ制限を設定（3カラムレイアウトが適切に表示されるサイズ）
    ImGui::SetNextWindowSizeConstraints(ImVec2(800.0f, 400.0f), ImVec2(FLT_MAX, FLT_MAX));

    if (ImGui::Begin("Work Manager", &m_isVisible))
    {
        // --- 1. Workの新規作成 ---
        ImGui::InputText("New Work Name", m_newWorkNameBuffer, IM_ARRAYSIZE(m_newWorkNameBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Create Work") && strlen(m_newWorkNameBuffer) > 0)
        {
            auto newWork = WorkManager::GetInstance().CreateWork(m_newWorkNameBuffer);
            m_selectedWork = newWork; // 作成したものを選択
            m_selectedWorkflow = nullptr;
            m_newWorkNameBuffer[0] = '\0';
        }

        ImGui::Separator();

        // --- 2. メインレイアウト ---
        // 左カラムに「Works」と「Workflows in」を縦に並べ、右カラムに「Workflow:」(Task管理) を配置する
        ImGui::Columns(2, "WorkManagerLayout", true);

        // 左カラムの上下を均等にする高さ
        const float leftColumnHeight = ImGui::GetContentRegionAvail().y;
        const float halfPaneHeight = (leftColumnHeight > 0.0f) ? (leftColumnHeight * 0.5f) : 0.0f;

        // 左カラム: Work一覧 + WorkFlow一覧（縦並び・均等高さ）
        DrawWorkListColumn(halfPaneHeight);
        DrawWorkFlowColumn(halfPaneHeight);
        ImGui::NextColumn();

        // 右カラム: Taskの管理 (Workflowへの追加 / 新規Task作成)
        DrawTaskColumn();
        ImGui::Columns(1);
    }
    ImGui::End();
}

// カラム1: Work一覧の描画
void DrawWorkManagerWindow::DrawWorkListColumn(float paneHeightY)
{
    const ImVec2 childSize(0.0f, paneHeightY > 0.0f ? paneHeightY : 0.0f);
    ImGui::BeginChild("WorkListPane", childSize, false);
    ImGui::Text("Works");
    ImGui::Separator();

    const auto& works = WorkManager::GetInstance().GetAllWorks();
    Work* workToDelete = nullptr;

    for (const auto& work : works)
    {
        ImGui::PushID(work.get());
        bool isSelected = (m_selectedWork == work.get());
        if (ImGui::Selectable(work->m_name.c_str(), isSelected))
        {
            m_selectedWork = work.get();
            m_selectedWorkflow = nullptr; // Workを切り替えたらWorkflow選択もリセット
        }

        // TODO:Web系のComponent指向を取り入れてもいいかもね

        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(10, 10)))
        {
            workToDelete = work.get();
        }
        
        ImGui::PopID();
    }

    if (workToDelete)
    {
        if (m_selectedWork == workToDelete) {
            m_selectedWork = nullptr;
            m_selectedWorkflow = nullptr;
        }
        WorkManager::GetInstance().DeleteWork(workToDelete);
    }

    ImGui::Separator();
    
    // そのWorkが開始する条件の表示
    ImGui::Text("Trigger Condition");
    if (m_selectedWork && m_selectedWork->m_startCondition)
    {
        m_selectedWork->m_startCondition->DrawInspectorUI();
    }
    else
    {
        ImGui::Text("No Start Condition Set");
    }

    ImGui::Separator();
    
    // そのWorkが完了したときの報酬アクションの表示
    ImGui::Text("Reward Actions");
    if (m_selectedWork && !m_selectedWork->m_rewardActions.empty())
    {
        for (const auto& action : m_selectedWork->m_rewardActions)
        {
            if (action)
            {
                ImGui::Text("Action: %s", action->GetName().c_str());
                action->DrawInspectorUI();
                ImGui::Separator();
            }
        }
    }
    else
    {
        ImGui::Text("No Reward Actions Set");
    }

    // 編集可能な開始条件と報酬アクションの設定UI
    if (m_selectedWork)
    {
        ImGui::Separator();
        ImGui::Text("Edit Start Condition");
        // 開始条件の編集UI
        auto& factory = TriggerFactory::GetInstance();
        auto conditionNames = factory.GetRegisteredConditionNames();
        static std::string selectedStartConditionName;
        if (selectedStartConditionName.empty() && !conditionNames.empty()) {
            selectedStartConditionName = conditionNames[0];
        }
        if (ImGui::BeginCombo("Start Condition", selectedStartConditionName.c_str()))
        {
            for (const auto& name : conditionNames)
            {
                if (ImGui::Selectable(name.c_str(), selectedStartConditionName == name)) {
                    selectedStartConditionName = name;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Set Start Condition"))
        {
            m_selectedWork->m_startCondition = factory.CreateCondition(selectedStartConditionName);
        }

        ImGui::Separator();
        ImGui::Text("Add Reward Action");
        auto actionNames = factory.GetRegisteredActionNames();
        static std::string selectedRewardActionName;
        if (selectedRewardActionName.empty() && !actionNames.empty()) {
            selectedRewardActionName = actionNames[0];
        }
        if (ImGui::BeginCombo("Reward Action", selectedRewardActionName.c_str()))
        {
            for (const auto& name : actionNames)
            {
                if (ImGui::Selectable(name.c_str(), selectedRewardActionName == name)) {
                    selectedRewardActionName = name;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Add Reward Action"))
        {
            m_selectedWork->m_rewardActions.push_back(factory.CreateAction(selectedRewardActionName));
        }
    }
    
    ImGui::EndChild();
}

// カラム2: WorkFlow一覧の描画
void DrawWorkManagerWindow::DrawWorkFlowColumn(float paneHeightY)
{
    const ImVec2 childSize(0.0f, paneHeightY > 0.0f ? paneHeightY : 0.0f);
    ImGui::BeginChild("WorkFlowListPane", childSize, false);
    if (!m_selectedWork)
    {
        ImGui::Text("Select a Work");
        ImGui::EndChild();
        return;
    }

    ImGui::Text("Workflows in [%s]", m_selectedWork->m_name.c_str());
    ImGui::Separator();

    // --- 新規WorkFlow作成 ---
    ImGui::InputText("New Workflow", m_newFlowNameBuffer, IM_ARRAYSIZE(m_newFlowNameBuffer));
    ImGui::SameLine();
    if (ImGui::Button("Add") && strlen(m_newFlowNameBuffer) > 0)
    {
        m_selectedWork->m_workflows.push_back(std::make_unique<WorkFlow>(m_newFlowNameBuffer));
        m_newFlowNameBuffer[0] = '\0';
    }
    ImGui::Separator();

    // --- WorkFlowリスト (並べ替え対応) ---
    WorkFlow* wfToDelete = nullptr;
    int wfToMove = -1;
    int wfMoveDir = 0; // -1: up, 1: down

    for (int i = 0; i < m_selectedWork->m_workflows.size(); ++i)
    {
        auto wf = m_selectedWork->m_workflows[i].get();
        ImGui::PushID(wf);

        bool isSelected = (m_selectedWorkflow == wf);
        if (ImGui::Selectable(wf->m_name.c_str(), isSelected))
        {
            m_selectedWorkflow = std::move(wf);
        }

        // 並べ替えボタン
        ImGui::SameLine();
        if (ImGui::ArrowButton("##up", ImGuiDir_Up) && i > 0) {
            wfToMove = i;
            wfMoveDir = -1;
        }
        ImGui::SameLine();
        if (ImGui::ArrowButton("##down", ImGuiDir_Down) && i < m_selectedWork->m_workflows.size() - 1) {
            wfToMove = i;
            wfMoveDir = 1;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("X")) { wfToDelete = wf; }

        ImGui::PopID();
    }

    // 並べ替え処理
    if (wfToMove != -1) {
        std::swap(m_selectedWork->m_workflows[wfToMove], m_selectedWork->m_workflows[wfToMove + wfMoveDir]);
    }

    // 削除処理
    if (wfToDelete)
    {
        if (m_selectedWorkflow == wfToDelete) m_selectedWorkflow = nullptr;
        auto& wfs = m_selectedWork->m_workflows;
        wfs.erase(std::remove_if(wfs.begin(), wfs.end(), 
                  [wfToDelete](const std::unique_ptr<WorkFlow>& wf){ return wf.get() == wfToDelete; }), 
                  wfs.end());
    }

    ImGui::EndChild();
}

// カラム3: Task管理の描画
void DrawWorkManagerWindow::DrawTaskColumn()
{
    ImGui::BeginChild("TaskColumnPane");
    if (!m_selectedWorkflow)
    {
        ImGui::Text("Select a Workflow");
        ImGui::EndChild();
        return;
    }

    ImGui::Text("Workflow: %s", m_selectedWorkflow->m_name.c_str());

    // --- 実行モード (Sequential / Parallel) ---
    const char* modes[] = { "Sequential", "Parallel" };
    int current_mode = static_cast<int>(m_selectedWorkflow->m_mode);
    if (ImGui::Combo("Mode", &current_mode, modes, IM_ARRAYSIZE(modes)))
    {
        m_selectedWorkflow->m_mode = static_cast<EWorkFlowMode>(current_mode);
    }

    ImGui::Separator();

    // --- 4. TaskをWorkFlowに追加 (ドラッグ＆ドロップ先) ---
    ImGui::Text("Tasks in this Workflow (Drag to reorder)");
    ImGui::BeginChild("WorkflowTasks", ImVec2(0, 150), true);
    
    int task_to_remove = -1;
    for (int i = 0; i < m_selectedWorkflow->m_tasks.size(); ++i)
    {
        auto* task = m_selectedWorkflow->m_tasks[i];

        // ID はインデックスベースにして、nullptr（Noneタスク）でも扱えるようにする
        ImGui::PushID(i);
        
        std::string task_label;
        if (task)
        {
            task_label = task->GetTaskName();
            if (task->gameObject)
                task_label += " (on " + task->gameObject->GetName() + ")";
        }
        else
        {
            task_label = "None (Empty Task)";
        }

        ImGui::Selectable(task_label.c_str());

        // ドラッグ＆ドロップ (並べ替え用)
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("TASK_REORDER_PAYLOAD", &i, sizeof(int));
            ImGui::Text("Move %s", task_label.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TASK_REORDER_PAYLOAD"))
            {
                int sourceIndex = *static_cast<const int*>(payload->Data);
                if (sourceIndex != i) {
                    std::swap(m_selectedWorkflow->m_tasks[sourceIndex], m_selectedWorkflow->m_tasks[i]);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 20);
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(20, 20))) { task_to_remove = i; }

        ImGui::PopID();
    }
    if (task_to_remove != -1) {
        m_selectedWorkflow->m_tasks.erase(m_selectedWorkflow->m_tasks.begin() + task_to_remove);
    }
    ImGui::EndChild();

    // WorkFlowタスクリスト全体をドロップターゲットにする
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TASK_PAYLOAD"))
        {
            // ペイロードには &task でコピーした「ポインタの値」が入っているので、1回デリファレンスする
            TriggerComponent* receivedTask =
                *static_cast<TriggerComponent* const*>(payload->Data);

            // 既に追加されていないかチェック
            bool alreadyAdded = false;
            for(const auto t : m_selectedWorkflow->m_tasks) {
                if (t == receivedTask) {
                    alreadyAdded = true;
                    break;
                }
            }
            if (!alreadyAdded) {
                m_selectedWorkflow->m_tasks.push_back(receivedTask);
            }
        }
        ImGui::EndDragDropTarget();
    }
    
    ImGui::Separator();

    // --- 利用可能なTask一覧 (ドラッグ元) ---
    ImGui::Text("Available Tasks in Scene (Drag to add)");
    ImGui::BeginChild("AvailableTasks", ImVec2(0, 150), true);
    for (const auto task : m_sceneTriggersCache)
    {
        // 既にこのWorkFlowに含まれているタスクはグレーアウト
        const bool in_workflow = std::find(m_selectedWorkflow->m_tasks.begin(), m_selectedWorkflow->m_tasks.end(), task)
            != m_selectedWorkflow->m_tasks.end();
        
        std::string availableLabel = task->GetTaskName();
        if (task->gameObject)
            availableLabel += " (on " + task->gameObject->GetName() + ")";

        if (in_workflow) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::Selectable(availableLabel.c_str());
            ImGui::PopStyleColor();
        } else {
            // ドラッグ可能なアイテムとして設定
            ImGui::Selectable(availableLabel.c_str());
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("TASK_PAYLOAD", &task, sizeof(TriggerComponent*)); 
                ImGui::Text("Add %s", task->GetTaskName().c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

// ヘルパー：シーン内の全TriggerComponentをキャッシュに格納
void DrawWorkManagerWindow::RefreshTriggerCache()
{
    m_sceneTriggersCache.clear();

    for (const auto& obj : g_Scene->GetGameObjects())
    {
        // GameObject::GetComponentsOfType<T>() が `std::vector<T*>` を返すと仮定
        const auto triggers = obj->FindComponents<TriggerComponent>(); 
        m_sceneTriggersCache.insert(m_sceneTriggersCache.end(), triggers.begin(), triggers.end());
    }
}