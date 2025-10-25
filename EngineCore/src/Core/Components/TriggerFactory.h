#pragma once
#include <string>
#include <memory>
#include <functional>
#include <map>
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/Reward/IReward.h"

class TriggerFactory
{
public:
    // シングルトンインスタンス
    static TriggerFactory& GetInstance();

    // Conditionを名前から生成
    std::unique_ptr<ITriggerCondition> CreateCondition(const std::string& name);
    // Actionを名前から生成
    std::unique_ptr<IReward> CreateAction(const std::string& name);

    // 生成可能なクラスを登録する
    template<typename T>
    void RegisterCondition(const std::string& name) {
        m_ConditionRegistry[name] = []() { return std::make_unique<T>(); };
    }
    template<typename T>
    void RegisterAction(const std::string& name) {
        m_ActionRegistry[name] = []() { return std::make_unique<T>(); };
    }

    // 登録されているConditionの名前リストを取得
    std::vector<std::string> GetRegisteredConditionNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : m_ConditionRegistry) {
            names.push_back(pair.first);
        }
        return names;
    }

    // 登録されているActionの名前リストを取得
    std::vector<std::string> GetRegisteredActionNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : m_ActionRegistry) {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    TriggerFactory() = default;
    std::map<std::string, std::function<std::unique_ptr<ITriggerCondition>()>> m_ConditionRegistry;
    std::map<std::string, std::function<std::unique_ptr<IReward>()>> m_ActionRegistry;
};