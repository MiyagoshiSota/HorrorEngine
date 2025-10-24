#pragma once
#include "IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Core/Components/TriggerComponent.h"
#include "Core/Components/TriggerFactory.h"

class DrawTaskManagerWindow : public IDrawWindow
{
public:
    std::shared_ptr<GameObject> s_TargetForNewTask = nullptr; 
    std::string s_SelectedConditionName = "";
    std::string s_SelectedActionName = "";
    
public:
    void draw() override
    {
        ImGui::Begin("Task Manager");

        auto& factory = TriggerFactory::GetInstance();

        ImGui::Text("Existing Tasks");
        ImGui::Separator();
        
        // 全GameObjectをスキャンしてTriggerComponentを収集
        std::map<std::shared_ptr<GameObject>, std::vector<std::shared_ptr<TriggerComponent>>> taskMap;
        for (const auto& obj : g_Scene->get_game_objects())
        {
            for (const auto& comp : obj->components)
            {
                if (comp->get_type() == "Trigger")
                {
                    std::shared_ptr<TriggerComponent> trigger = std::dynamic_pointer_cast<TriggerComponent>(comp);
                    if (trigger)
                    {
                        taskMap[obj].push_back(trigger);
                    }
                }
            }
        }

        // 収集したデータを元にGUIを描画
        for (auto& [gameObject, triggers] : taskMap)
        {
            // Target (GameObject)
            if (ImGui::TreeNode(gameObject.get(), "Target: %s", gameObject->get_name().c_str()))
            {
                for (int i = 0; i < triggers.size(); ++i)
                {
                    std::shared_ptr<TriggerComponent> trigger = triggers[i];
                    ImGui::PushID(trigger.get()); // 各TriggerComponentを一意に識別

                    ImGui::Text("Task %d", i + 1);
                    ImGui::SameLine();

                    // 削除ボタン
                    if (ImGui::Button("Delete Task"))
                    {
                        gameObject->RemoveComponent(trigger);
                        ImGui::PopID();
                        break; // vectorが変更されるのでループを抜ける
                    }

                    trigger->DrawInspectorUI(); 

                    ImGui::Separator();
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }
            
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Create New Task");
        
        // [Target] : GameObjectを選択
        std::string currentTargetName = s_TargetForNewTask ? s_TargetForNewTask->get_name() : "None";
        if (ImGui::BeginCombo("Target", currentTargetName.c_str()))
        {
            // "None"選択肢
            if (ImGui::Selectable("None", s_TargetForNewTask == nullptr))
            {
                s_TargetForNewTask = nullptr;
            }
            
            // シーン内の全オブジェクト
            for (const auto& obj : g_Scene->get_game_objects())
            {
                bool is_selected = (s_TargetForNewTask == obj);
                if (ImGui::Selectable(obj->get_name().c_str(), is_selected))
                {
                    s_TargetForNewTask = obj;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // [Action] (Condition) を選択
        auto conditionNames = factory.GetRegisteredConditionNames();
        if (s_SelectedConditionName.empty() && !conditionNames.empty()) s_SelectedConditionName = conditionNames[0];
        if (ImGui::BeginCombo("Action (Condition)", s_SelectedConditionName.c_str()))
        {
            for (const auto& name : conditionNames)
            {
                if (ImGui::Selectable(name.data(), s_SelectedConditionName == name)) s_SelectedConditionName = name;
            }
            ImGui::EndCombo();
        }
        
        // [Reward] (IAction) を選択
        auto actionNames = factory.GetRegisteredActionNames();
        if (s_SelectedActionName.empty() && !actionNames.empty()) s_SelectedActionName = actionNames[0];
        if (ImGui::BeginCombo("Reward (IAction)", s_SelectedActionName.c_str()))
        {
            for (const auto& name : actionNames)
            {
                if (ImGui::Selectable(name.data(), s_SelectedActionName == name)) s_SelectedActionName = name;
            }
            ImGui::EndCombo();
        }

        // 作成ボタン
        if (ImGui::Button("Create Task"))
        {
            if (s_TargetForNewTask && !s_SelectedConditionName.empty() && !s_SelectedActionName.empty())
            {
                // TargetのGameObjectにTriggerComponentを追加
                std::shared_ptr<TriggerComponent> newTrigger = s_TargetForNewTask->AddComponent<TriggerComponent>();
                
                // Condition と Action をファクトリで生成して設定
                newTrigger->Condition = factory.CreateCondition(s_SelectedConditionName);
                
                auto newAction = factory.CreateAction(s_SelectedActionName);
                if(newAction)
                {
                    newTrigger->Actions.push_back(std::move(newAction));
                }

                newTrigger->gameObject = s_TargetForNewTask;
            }
        }
        ImGui::End();
    }
};
