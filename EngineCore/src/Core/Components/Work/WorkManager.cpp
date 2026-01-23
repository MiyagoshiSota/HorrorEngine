#include "WorkManager.h"
#include "Work.h"
#include <algorithm>

#include "Core/App.h"

WorkManager& WorkManager::GetInstance()
{
    static WorkManager instance;
    return instance;
}

void WorkManager::Update(float deltaTime)
{
    m_context.m_DeltaTime = deltaTime;
    
    // すべてのWorkをイテレートし、アクティブなものを更新
    for (auto& work : m_works)
    {
        if (work && !work->m_isComplete)
        {
            work->Update(m_context);
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
                return workPtr.get() == workToDelete;
            });

    if (it != m_works.end())
    {
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

    for (const auto& obj : g_Scene->GetGameObjects())
    {
        if (!obj) continue;
        // GameObjectからTriggerComponentのリストを取得
        auto foundTriggers = obj->FindComponents<TriggerComponent>(); 
        triggers.insert(triggers.end(), foundTriggers.begin(), foundTriggers.end());
    }
    return triggers;
}