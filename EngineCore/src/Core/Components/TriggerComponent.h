#pragma once
#include <vector>
#include <memory>

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE
#include "TriggerFactory.h"
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/Reward/IReward.h"
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

        // Trigger
        if (jsonData.contains("Trigger") && jsonData["Trigger"].contains("name"))
        {
            std::string conditionName = jsonData["Trigger"]["name"];
            // ファクトリに名前を渡して、対応するConditionのインスタンスを生成
            Condition = TriggerFactory::GetInstance().CreateCondition(conditionName);
        }

        // Action
        if (jsonData.contains("Action") && jsonData["Action"].contains("name"))
        {
            const std::string action_name = jsonData["Action"]["name"];
            // ファクトリに名前を渡して、対応するActionのインスタンスを生成
            if (auto new_action = TriggerFactory::GetInstance().CreateAction(action_name)) {
                Actions.push_back(std::move(new_action));
            }
        }
    };
    

    std::string GetType() override
    {
        return "Trigger";
    };

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

        // === Condition の設定 UI ===
        {
            // ファクトリからCondition名のリストを取得
            auto conditionNames = factory.GetRegisteredConditionNames();
            // Conditionを解除するための "None" を追加
            conditionNames.insert(conditionNames.begin(), "None");

            // 現在設定されているConditionの名前を取得
            std::string currentConditionName = Condition ? Condition->GetName() : "None";

            // コンボボックス（ドロップダウンリスト）
            if (ImGui::BeginCombo("Trigger Condition", currentConditionName.c_str()))
            {
                for (const auto& name : conditionNames)
                {
                    bool is_selected = (currentConditionName == name);
                    if (ImGui::Selectable(name.c_str(), is_selected))
                    {
                        // 選択が変更された場合
                        if (name == "None")
                        {
                            Condition = nullptr; // Noneが選ばれたらnullptrに
                        }
                        else if (currentConditionName != name)
                        {
                            // 違うものが選ばれたらファクトリで生成して差し替える
                            Condition = factory.CreateCondition(name);
                        }
                    }
                    if (is_selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Separator();

        // === Actions の設定 UI ===
        {
            ImGui::Text("Actions");

            // --- 現在のActionリストを削除ボタン付きで表示 ---
            int actionToRemove = -1; // 削除対象のインデックス
            for (int i = 0; i < Actions.size(); ++i)
            {
                if (!Actions[i]) continue;

                ImGui::PushID(i); // ImGuiが要素を区別するためのID
                ImGui::Text("- %s", Actions[i]->GetName().c_str());
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    actionToRemove = i; // 削除ボタンが押されたらインデックスを記録
                }
                ImGui::PopID();
            }

            // ループの外で安全に削除
            if (actionToRemove != -1)
            {
                Actions.erase(Actions.begin() + actionToRemove);
            }

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

