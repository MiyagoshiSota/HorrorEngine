#pragma once
#include <vector>
#include <memory>

#include "TriggerFactory.h"
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/Action/IAction.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/GameObject/Component/Component.h"

class TriggerComponent : public Component
{
public:
    // GUIから動的に設定される
    std::unique_ptr<ITriggerCondition> Condition;
    std::vector<std::unique_ptr<IAction>> Actions;

    // start
    void start() override
    {
        context.m_Owner = gameObject;
    }
    
    // 毎フレーム呼ばれる
    void update(float deltaTime) override
    {
        if (Condition && Condition->Check(context))
        {
            for (auto& action : Actions)
            {
                action->Execute(context);
            }
        }
    }

    void deserialize(const nlohmann::json& jsonData, std::shared_ptr<GameObject> game_object) override
    {
        // 親ゲームオブジェクトを設定
        gameObject = game_object;
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
    
    void DrawInspectorUI(); // ConditionとActionsのUIを描画する

    std::string get_type() override
    {
        return "Trigger";
    };

    std::string get_trigger_name()
    {
        if (Condition)
        {
            return Condition->GetName();
        }
        return "No Trigger";
    }
    
private:
    TriggerContext context;
};
