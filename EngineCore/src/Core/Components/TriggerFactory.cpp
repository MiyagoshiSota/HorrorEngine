#include "TriggerFactory.h"

TriggerFactory& TriggerFactory::GetInstance()
{
    // staticなローカル変数としてインスタンスを生成
    // これにより、プログラム全体で唯一のインスタンスであることが保証される
    static TriggerFactory instance;
    return instance;
}

std::unique_ptr<ITriggerCondition> TriggerFactory::CreateCondition(const std::string& name)
{
    // 登録されたConditionのマップ(m_ConditionRegistry)から名前を検索
    auto it = m_ConditionRegistry.find(name);

    // もし名前が見つかったら
    if (it != m_ConditionRegistry.end())
    {
        // 関連付けられた生成関数 (it->second) を呼び出してインスタンスを生成し、返す
        return it->second();
    }

    // 名前が見つからなければ、nullptrを返す
    printf("Error: Condition type '%s' not registered in factory.\n", name.c_str());
    return nullptr;
}

std::unique_ptr<IAction> TriggerFactory::CreateAction(const std::string& name)
{
    // 登録されたActionのマップ(m_ActionRegistry)から名前を検索
    auto it = m_ActionRegistry.find(name);

    //もし名前が見つかったら
    if (it != m_ActionRegistry.end())
    {
        // 関連付けられた生成関数を呼び出してインスタンスを生成し、返す
        return it->second();
    }
    
    // 名前が見つからなければ、nullptrを返す
    printf("Error: Action type '%s' not registered in factory.\n", name.c_str());
    return nullptr;
}