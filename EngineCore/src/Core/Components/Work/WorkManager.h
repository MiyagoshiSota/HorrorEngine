#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Work.h"

class Scene;
class TriggerComponent;
struct TriggerContext;

class WorkManager
{
public:
    static WorkManager& GetInstance();

    WorkManager(const WorkManager&) = delete;
    WorkManager& operator=(const WorkManager&) = delete;
    
    void Update() const;
    
    Work* CreateWork(const std::string& name);
    
    void DeleteWork(Work* workToDelete);
    
    std::vector<std::unique_ptr<Work>>& GetAllWorks();
    
    std::vector<std::shared_ptr<TriggerComponent>> FindAllTriggersInScene();

private:
    WorkManager() = default;  // プライベートコンストラクタ
    ~WorkManager() = default; // プライベートデストラクタ

    std::vector<std::unique_ptr<Work>> m_works; // すべてのWorkを所有
};