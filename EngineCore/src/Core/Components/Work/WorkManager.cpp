#include "WorkManager.h"
#include "Work.h"
#include <algorithm>

#include "Core/App.h"

WorkManager& WorkManager::GetInstance()
{
    static WorkManager instance;
    return instance;
}

void WorkManager::Update() const
{
    // すべてのWorkをイテレートし、アクティブなものを更新
    for (auto& work : m_works)
    {
        if (work && !work->m_isComplete)
        {
            work->Update();
        }
    }
}

Work* WorkManager::CreateWork(const std::string& name)
{
    m_works.push_back(std::make_unique<Work>(name));
    return m_works.back().get();
}

void WorkManager::DeleteWork(Work* workToDelete)
{
    const auto it = std::remove_if(m_works.begin(), m_works.end(),
            [workToDelete](const std::unique_ptr<Work>& workPtr) 
            {
                // unique_ptrが持つ生のポインタと、削除対象の生のポインタを比較する
                return workPtr.get() == workToDelete;
            });

    if (it != m_works.end())
    {
        // vectorの末尾に移動した要素（たち）を削除する
        // これによりunique_ptrのデストラクタが呼ばれ、Workオブジェクトが解放される
        m_works.erase(it, m_works.end());
    }
}

std::vector<std::unique_ptr<Work>>& WorkManager::GetAllWorks()
{
    return m_works;
}

std::vector<std::shared_ptr<TriggerComponent>> WorkManager::FindAllTriggersInScene()
{
    std::vector<std::shared_ptr<TriggerComponent>> triggers;

    for (const auto& obj : g_Scene->get_game_objects())
    {
        if (!obj) continue;
        // GameObjectからTriggerComponentのリストを取得
        auto foundTriggers = obj->find_components<TriggerComponent>(); 
        triggers.insert(triggers.end(), foundTriggers.begin(), foundTriggers.end());
    }
    return triggers;
}