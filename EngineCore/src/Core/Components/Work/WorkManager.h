#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Work.h"
#include <nlohmann/json.hpp>

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

    /// Day(Scene)切り替え時: 現在のWorksを破棄する
    void Clear();

    /// 現在のWorksをシーン用JSONにシリアライズする（Task参照は gameObjectName + triggerIndex）
    nlohmann::json SerializeToSceneJson(const std::vector<std::shared_ptr<class GameObject>>& gameObjects) const;

    /// シーンJSONからWorksを復元する（Day(Scene)に紐づくデータとして読み込む）
    void LoadFromSceneJson(const nlohmann::json& sceneJson, const std::vector<std::shared_ptr<class GameObject>>& gameObjects);

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