#pragma once
#include <vector>
#include <memory>

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE
#include "TriggerFactory.h"
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/Reward/IReward.h"
#include "Core/Components/Reward/PlaySoundReward.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/GameObject/Component/Component.h"

class TriggerComponent : public Component
{
public:
    // GUIから動的に設定される
    std::unique_ptr<ITriggerCondition> Condition;
    std::vector<std::unique_ptr<IReward>> Actions;

    void Initialize(std::shared_ptr<GameObject> game_object) override
    {
        Component::Initialize(game_object);
    };

    // start
    void start() override
    {
        context.m_Owner = gameObject;
        context.m_DeltaTime = 0;
    }
    
    // 毎フレーム呼ばれる
    void update(float deltaTime) override
    {
        context.m_DeltaTime = deltaTime;
        
        if (m_isActivated && !m_isCompleted) 
        {
            if (Condition && Condition->Check(context))
            {
                for (auto& action : Actions)
                {
                    action->Execute(context);
                }
                m_isCompleted = true; // 実行したら完了フラグを立てる
            }
        }
    }

    void Deserialize(const nlohmann::json& jsonData) override
    {
        // 親ゲームオブジェクトを設定
        context = TriggerContext();
        context.m_Owner = gameObject;

        // Task名（Workflow内での表示名）
        if (jsonData.contains("taskName"))
            m_taskName = jsonData["taskName"].get<std::string>();

        // Trigger (Condition)
        if (jsonData.contains("Trigger") && jsonData["Trigger"].contains("name"))
        {
            std::string conditionName = jsonData["Trigger"]["name"].get<std::string>();
            Condition = TriggerFactory::GetInstance().CreateCondition(conditionName);
            if (Condition)
                Condition->Deserialize(jsonData["Trigger"]);
        }

        // Actions（複数。保存キーは "Actions" 配列）
        if (jsonData.contains("Actions") && jsonData["Actions"].is_array())
        {
            for (const auto& actionJson : jsonData["Actions"])
            {
                if (!actionJson.contains("name")) continue;
                std::string actionType = actionJson["name"].get<std::string>();
                if (auto action = TriggerFactory::GetInstance().CreateAction(actionType))
                {
                    if (actionType == "StartWorkReward" && actionJson.contains("workName"))
                    {
                        auto* sw = dynamic_cast<StartWorkReward*>(action.get());
                        if (sw) sw->SetPendingWorkName(actionJson["workName"].get<std::string>());
                    }
                    if (actionType == "PlaySoundAction")
                    {
                        auto* ps = dynamic_cast<PlaySoundReward*>(action.get());
                        if (ps)
                        {
                            if (actionJson.contains("soundName"))
                                ps->SetSoundName(actionJson["soundName"].get<std::string>());
                            if (actionJson.contains("use3d"))
                                ps->SetUse3d(actionJson["use3d"].get<bool>());
                        }
                    }
                    Actions.push_back(std::move(action));
                }
            }
        }
        else if (jsonData.contains("Action") && jsonData["Action"].contains("name"))
        {
            // 旧形式: 単一 "Action"
            std::string actionType = jsonData["Action"]["name"].get<std::string>();
            if (auto action = TriggerFactory::GetInstance().CreateAction(actionType))
                Actions.push_back(std::move(action));
        }
    };
    

    std::string GetType() override
    {
        return "Trigger";
    };

    /// シーンロード後、全 TriggerComponent の StartWorkReward の pending workName を解決する
    static void ResolvePendingWorkReferencesInScene(const std::vector<std::shared_ptr<class GameObject>>& gameObjects);

    std::string get_trigger_name() const
    {
        if (Condition)
        {
            return Condition->GetName();
        }
        return "No Trigger";
    }

#ifndef BUILD_STANDALONE
    void OnGui() override
    {
        ImGui::Text("Trigger Component");
        ImGui::Separator();
        
        ImGui::Text("Trigger: %s", get_trigger_name().c_str());
        if (Condition) Condition->DrawInspectorUI();
        
        for (auto & action : Actions)
        {
            ImGui::Text("Action: %s", action->GetName().c_str());
            action->DrawInspectorUI();
        }
        
        ImGui::Separator();
        
        ImGui::Checkbox("Is Activated", &m_isActivated);
    
        ImGui::Separator();
    
        // GUIから動的に設定するためのUIを描画
        DrawInspectorUI();
    }
#endif // BUILD_STANDALONE

#ifndef BUILD_STANDALONE
    void DrawInspectorUI()
    {
        auto& factory = TriggerFactory::GetInstance();

        // === Condition の設定 UI（TreeNode） ===
        {
            auto conditionNames = factory.GetRegisteredConditionNames();
            conditionNames.insert(conditionNames.begin(), "None");
            std::string currentConditionName = Condition ? Condition->GetName() : "None";

            if (ImGui::TreeNode(this, "Trigger (Condition): %s", currentConditionName.c_str()))
            {
                if (ImGui::BeginCombo("Trigger Condition", currentConditionName.c_str()))
                {
                    for (const auto& name : conditionNames)
                    {
                        bool is_selected = (currentConditionName == name);
                        if (ImGui::Selectable(name.c_str(), is_selected))
                        {
                            if (name == "None")
                                Condition = nullptr;
                            else if (currentConditionName != name)
                                Condition = factory.CreateCondition(name);
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (Condition)
                    Condition->DrawInspectorUI();
                ImGui::TreePop();
            }
        }

        ImGui::Separator();

        // === Actions の設定 UI（各ActionをTreeNodeで表示） ===
        {
            ImGui::Text("Actions");

            int actionToRemove = -1;
            for (int i = 0; i < static_cast<int>(Actions.size()); ++i)
            {
                if (!Actions[i]) continue;

                ImGui::PushID(i);
                if (ImGui::TreeNode(Actions[i].get(), "Action: %s", Actions[i]->GetName().c_str()))
                {
                    if (ImGui::Button("Remove"))
                        actionToRemove = i;
                    Actions[i]->DrawInspectorUI();
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (actionToRemove != -1)
                Actions.erase(Actions.begin() + actionToRemove);

            // --- 新しいActionを追加するUI ---
            
            // 追加するActionを選択するためのコンボボックス
            auto actionNames = factory.GetRegisteredActionNames();
            
            // デフォルト選択
            if (m_actionToAddName.empty() && !actionNames.empty())
            {
                m_actionToAddName = actionNames[0];
            }
            
            if (ImGui::BeginCombo("##AddActionCombo", m_actionToAddName.c_str()))
            {
                for (const auto& name : actionNames)
                {
                    bool is_selected = (m_actionToAddName == name);
                    if (ImGui::Selectable(name.c_str(), is_selected))
                    {
                        m_actionToAddName = name;
                    }
                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::SameLine();

            // "Add"ボタン
            if (ImGui::Button("Add Action"))
            {
                if (!m_actionToAddName.empty())
                {
                    // ファクトリで生成してvectorの末尾に追加
                    if (auto newAction = factory.CreateAction(m_actionToAddName))
                    {
                        Actions.push_back(std::move(newAction));
                    }
                }
            }
        }
    }
#else
    void DrawInspectorUI()
    {
        // BUILD_STANDALONE時は何もしない
    }
#endif // BUILD_STANDALONE

public:
    void Activate() { m_isActivated = true; }
    void Deactivate() { m_isActivated = false; }
    void ResetTask() { m_isCompleted = false; m_isActivated = false; }
    bool IsCompleted() const { return m_isCompleted; }
    const std::string& GetTaskName() const { return m_taskName; }
    void SetTaskName(const std::string& name) { m_taskName = name; }
    
private:
    TriggerContext context;

    bool m_isActivated = true; 
    bool m_isCompleted = false; // 既に実行が完了したか
    std::string m_taskName = "Untitled Task"; // GUIで表示するための名前

private:
    std::string m_actionToAddName;
};

