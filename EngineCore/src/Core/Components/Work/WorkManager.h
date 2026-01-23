#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Work.h"

class Scene;
class TriggerComponent;

class WorkManager
{
public:
    static WorkManager& GetInstance();

    WorkManager(const WorkManager&) = delete;
    WorkManager& operator=(const WorkManager&) = delete;
    
    void Update(float deltaTime);
    
    Work* CreateWork(const std::string& name);
    
    void DeleteWork(Work* workToDelete);
    
    std::vector<std::unique_ptr<Work>>& GetAllWorks();
    
    std::vector<std::shared_ptr<TriggerComponent>> FindAllTriggersInScene();

private:
    WorkManager()
    {
        m_context.m_DeltaTime = 0.0f;
        m_context.m_Owner = nullptr;
    }  // プライベートコンストラクタ
    ~WorkManager() = default; // プライベートデストラクタ

    std::vector<std::unique_ptr<Work>> m_works; // すべてのWorkを所有
    TriggerContext m_context;
};