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
    :m_selectedWork(nullptr), m_selectedWorkflow(nullptr), m_selectedGameObjectIndex(-1)
{
    // バッファの初期化
    memset(m_newWorkNameBuffer, 0, sizeof(m_newWorkNameBuffer));
    memset(m_newFlowNameBuffer, 0, sizeof(m_newFlowNameBuffer));
    memset(m_newTaskNameBuffer, 0, sizeof(m_newTaskNameBuffer));
}

// 毎フレーム呼ばれるメインの描画関数
void DrawWorkManagerWindow::draw()
{
    // HACK:全部のTriggerを毎フレーム取得しているが、パフォーマンス的に問題があれば改善する
    RefreshTriggerCache();

    if (ImGui::Begin("Work Manager"))
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

        // --- 2. メインの3カラムレイアウト ---
        ImGui::Columns(3, "WorkManagerLayout", true);

        // カラム1: Work一覧
        DrawWorkListColumn();
        ImGui::NextColumn();

        // カラム2: 選択中WorkのWorkFlow一覧
        DrawWorkFlowColumn();
        ImGui::NextColumn();

        // カラム3: Taskの管理 (Workflowへの追加 / 新規Task作成)
        DrawTaskColumn();
        ImGui::Columns(1);
    }
    ImGui::End();
}

// カラム1: Work一覧の描画
void DrawWorkManagerWindow::DrawWorkListColumn()
{
    ImGui::BeginChild("WorkListPane");
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
void DrawWorkManagerWindow::DrawWorkFlowColumn()
{
    ImGui::BeginChild("WorkFlowListPane");
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
        const auto task = m_selectedWorkflow->m_tasks[i];
        if (!task) continue; // 安全確認

        ImGui::PushID(task.get());
        
        std::string task_label = task->GetTaskName() + " (on " + task->gameObject->get_name() + ")";
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
            std::shared_ptr<TriggerComponent>* ptr_to_shared_ptr =
                static_cast<std::shared_ptr<TriggerComponent>*>(payload->Data);

            std::shared_ptr<TriggerComponent> receivedTask = *ptr_to_shared_ptr;
            // 既に追加されていないかチェック
            bool alreadyAdded = false;
            for(auto t : m_selectedWorkflow->m_tasks) {
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
    for (auto task : m_sceneTriggersCache)
    {
        // 既にこのWorkFlowに含まれているタスクはグレーアウト
        const bool in_workflow = std::find(m_selectedWorkflow->m_tasks.begin(), m_selectedWorkflow->m_tasks.end(), task)
            != m_selectedWorkflow->m_tasks.end();
        
        if (in_workflow) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::Selectable((task->GetTaskName() + " (on " + task->gameObject->get_name() + ")").c_str());
            ImGui::PopStyleColor();
        } else {
            // ドラッグ可能なアイテムとして設定
            ImGui::Selectable((task->GetTaskName() + " (on " + task->gameObject->get_name() + ")").c_str());
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("TASK_PAYLOAD", &task, sizeof(TriggerComponent*)); 
                ImGui::Text("Add %s", task->GetTaskName().c_str());
                ImGui::EndDragDropSource();
            }
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // --- 3. GUIでTaskを作成 ---
    DrawTaskCreatorPanel();

    ImGui::EndChild();
}

// カラム3の下部：新しいTask (TriggerComponent) を作成するUI
void DrawWorkManagerWindow::DrawTaskCreatorPanel()
{
    ImGui::Text("Create New Task");
    auto& factory = TriggerFactory::GetInstance();
    
    // Task Name
    ImGui::InputText("Task Name", m_newTaskNameBuffer, IM_ARRAYSIZE(m_newTaskNameBuffer));
    
    // Target GameObject
    std::string currentTargetName = "Select GameObject...";
    if (m_selectedGameObjectIndex >= 0 && m_selectedGameObjectIndex < g_Scene->get_game_objects().size()) {
        currentTargetName = g_Scene->get_game_objects()[m_selectedGameObjectIndex]->get_name();
    }

    if (ImGui::BeginCombo("Target GameObject", currentTargetName.c_str()))
    {
        auto gameObjects = g_Scene->get_game_objects();
        for (int i = 0; i < gameObjects.size(); ++i)
        {
            bool isSelected = (m_selectedGameObjectIndex == i);
            if (ImGui::Selectable(gameObjects[i]->get_name().c_str(), isSelected))
            {
                m_selectedGameObjectIndex = i;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Action (Condition)
    auto conditionNames = factory.GetRegisteredConditionNames();
    if (m_selectedConditionName.empty() && !conditionNames.empty()) {
        m_selectedConditionName = conditionNames[0];
    }
    if (ImGui::BeginCombo("Trigger (Condition)", m_selectedConditionName.c_str()))
    {
        for (const auto& name : conditionNames)
        {
            if (ImGui::Selectable(name.c_str(), m_selectedConditionName == name)) {
                m_selectedConditionName = name;
            }
        }
        ImGui::EndCombo();
    }

    // Reward (Action)
    auto actionNames = factory.GetRegisteredActionNames();
    if (m_selectedActionName.empty() && !actionNames.empty()) {
        m_selectedActionName = actionNames[0];
    }
    if (ImGui::BeginCombo("Reward (Action)", m_selectedActionName.c_str()))
    {
        for (const auto& name : actionNames)
        {
            if (ImGui::Selectable(name.c_str(), m_selectedActionName == name)) {
                m_selectedActionName = name;
            }
        }
        ImGui::EndCombo();
    }
    
    // Create Button
    // ターゲットGameObjectが選択され、かつTask名が入力されている場合のみボタンを有効化
    bool canCreate = (m_selectedGameObjectIndex != -1 && strlen(m_newTaskNameBuffer) > 0);
    if (!canCreate) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    
    if (ImGui::Button("Create & Add to Selected Workflow") && canCreate)
    {
        std::shared_ptr<GameObject> targetObject = g_Scene->get_game_objects()[m_selectedGameObjectIndex];
        
        // GameObjectにTriggerComponentを追加
        std::shared_ptr<TriggerComponent> newTrigger = targetObject->AddComponent<TriggerComponent>(); // AddComponentはT*を返すと仮定

        // TargetComponentの初期化
        newTrigger->SetTaskName(m_newTaskNameBuffer);
        newTrigger->gameObject = targetObject;
        newTrigger->ResetTask();
        
        // ConditionとActionをFactoryから作成してセット
        if (!m_selectedConditionName.empty()) {
            newTrigger->Condition = factory.CreateCondition(m_selectedConditionName);
        }
        if (!m_selectedActionName.empty()) {
            newTrigger->Actions.push_back(factory.CreateAction(m_selectedActionName));
        }

        // 現在選択中のWorkflowにも追加する
        if(m_selectedWorkflow)
        {
            m_selectedWorkflow->m_tasks.push_back(newTrigger);
        }
        
        // UIの入力欄をリセット
        m_newTaskNameBuffer[0] = '\0';
        m_selectedGameObjectIndex = -1;
        // m_selectedConditionName と m_selectedActionName はリセットしない (連続作成のため)
    }

    if (!canCreate) {
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Please select a target GameObject and enter a Task Name.");
        }
    }
}

// ヘルパー：シーン内の全TriggerComponentをキャッシュに格納
void DrawWorkManagerWindow::RefreshTriggerCache()
{
    m_sceneTriggersCache.clear();

    for (const auto& obj : g_Scene->get_game_objects())
    {
        // GameObject::GetComponentsOfType<T>() が `std::vector<T*>` を返すと仮定
        auto triggers = obj->find_components<TriggerComponent>(); 
        m_sceneTriggersCache.insert(m_sceneTriggersCache.end(), triggers.begin(), triggers.end());
    }
}