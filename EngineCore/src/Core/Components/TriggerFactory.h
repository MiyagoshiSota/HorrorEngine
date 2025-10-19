#pragma once
#include <string>
#include <memory>
#include <functional>
#include <map>
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/Action/IAction.h"

class TriggerFactory
{
public:
    // シングルトンインスタンス
    static TriggerFactory& GetInstance();

    // Conditionを名前から生成
    std::unique_ptr<ITriggerCondition> CreateCondition(const std::string& name);
    // Actionを名前から生成
    std::unique_ptr<IAction> CreateAction(const std::string& name);

    // 生成可能なクラスを登録する
    template<typename T>
    void RegisterCondition(const std::string& name) {
        m_ConditionRegistry[name] = []() { return std::make_unique<T>(); };
    }
    template<typename T>
    void RegisterAction(const std::string& name) {
        m_ActionRegistry[name] = []() { return std::make_unique<T>(); };
    }

private:
    TriggerFactory() = default;
    std::map<std::string, std::function<std::unique_ptr<ITriggerCondition>()>> m_ConditionRegistry;
    std::map<std::string, std::function<std::unique_ptr<IAction>()>> m_ActionRegistry;
};