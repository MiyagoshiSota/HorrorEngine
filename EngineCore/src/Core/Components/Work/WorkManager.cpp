#include "WorkManager.h"
#include "Work.h"
#include <algorithm>
#include <map>

#include "Core/App.h"
#include "Core/Components/TriggerComponent.h"
#include "Core/Components/TriggerFactory.h"
#include "Core/Components/Reward/AddSceneObjectToItemListReward.h"
#include "Core/Components/Reward/PlaceHeldItemReward.h"
#include "Core/Components/Reward/PlaySoundReward.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Modules/PublicConst/ConstGameObjectSaveParamPref.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/SceneManager.h"
#include <nlohmann/json.hpp>

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

void WorkManager::Clear()
{
    m_works.clear();
}

namespace
{
    // TriggerComponent* -> (gameObjectName, triggerIndex). triggerIndex はそのGameObject内のTriggerの通し番号
    std::map<TriggerComponent*, std::pair<std::string, int>> BuildTriggerRefMap(
        const std::vector<std::shared_ptr<GameObject>>& gameObjects)
    {
        std::map<TriggerComponent*, std::pair<std::string, int>> map;
        for (const auto& obj : gameObjects)
        {
            if (!obj) continue;
            int triggerIndex = 0;
            for (const auto& comp : obj->components)
            {
                auto* trigger = dynamic_cast<TriggerComponent*>(comp.get());
                if (trigger)
                {
                    map[trigger] = { obj->GetName(), triggerIndex };
                    ++triggerIndex;
                }
            }
        }
        return map;
    }

    TriggerComponent* ResolveTaskRef(
        const nlohmann::json& taskRef,
        const std::vector<std::shared_ptr<GameObject>>& gameObjects)
    {
        if (!taskRef.contains(ConstGameObjectSaveParamPref::kTaskRefGameObjectName) ||
            !taskRef.contains(ConstGameObjectSaveParamPref::kTaskRefTriggerIndex))
            return nullptr;
        const std::string goName = taskRef[ConstGameObjectSaveParamPref::kTaskRefGameObjectName].get<std::string>();
        const int triggerIndex = taskRef[ConstGameObjectSaveParamPref::kTaskRefTriggerIndex].get<int>();
        for (const auto& obj : gameObjects)
        {
            if (!obj || obj->GetName() != goName) continue;
            int idx = 0;
            for (const auto& comp : obj->components)
            {
                auto* trigger = dynamic_cast<TriggerComponent*>(comp.get());
                if (trigger)
                {
                    if (idx == triggerIndex) return trigger;
                    ++idx;
                }
            }
            break;
        }
        return nullptr;
    }
}

nlohmann::json WorkManager::SerializeToSceneJson(const std::vector<std::shared_ptr<GameObject>>& gameObjects) const
{
    const auto triggerRefMap = BuildTriggerRefMap(gameObjects);

    nlohmann::json worksJson = nlohmann::json::array();
    for (const auto& work : m_works)
    {
        if (!work) continue;
        nlohmann::json workJson;
        workJson[ConstGameObjectSaveParamPref::kWorkName] = work->m_name;

        if (work->m_startCondition)
            workJson[ConstGameObjectSaveParamPref::kWorkStartCondition] = work->m_startCondition->GetName();
        else
            workJson[ConstGameObjectSaveParamPref::kWorkStartCondition] = nullptr;

        nlohmann::json actionsJson = nlohmann::json::array();
        for (const auto& action : work->m_rewardActions)
        {
            if (!action) continue;
            nlohmann::json actionJson;
            actionJson[ConstGameObjectSaveParamPref::kRewardActionType] = action->GetName();
            auto* startWork = dynamic_cast<StartWorkReward*>(action.get());
            if (startWork && startWork->GetWork())
                actionJson[ConstGameObjectSaveParamPref::kRewardActionWorkName] = startWork->GetWork()->m_name;
            auto* addSceneObjReward = dynamic_cast<AddSceneObjectToItemListReward*>(action.get());
            if (addSceneObjReward && !addSceneObjReward->GetObjectName().empty())
                actionJson[ConstGameObjectSaveParamPref::kRewardActionGameObjectName] = addSceneObjReward->GetObjectName();
            auto* placeHeldReward = dynamic_cast<PlaceHeldItemReward*>(action.get());
            if (placeHeldReward)
            {
                const auto& pos = placeHeldReward->GetPosition();
                actionJson[ConstGameObjectSaveParamPref::kRewardActionPlacePosition] = { pos.x, pos.y, pos.z };
                if (!placeHeldReward->GetTargetObjectName().empty())
                    actionJson[ConstGameObjectSaveParamPref::kRewardActionPlaceTargetObject] = placeHeldReward->GetTargetObjectName();
            }
            auto* playSoundReward = dynamic_cast<PlaySoundReward*>(action.get());
            if (playSoundReward)
            {
                actionJson[ConstGameObjectSaveParamPref::kRewardActionSoundName] = playSoundReward->GetSoundName();
                actionJson[ConstGameObjectSaveParamPref::kRewardActionSoundUse3d] = playSoundReward->GetUse3d();
            }
            actionsJson.push_back(std::move(actionJson));
        }
        workJson[ConstGameObjectSaveParamPref::kWorkRewardActions] = std::move(actionsJson);

        nlohmann::json workflowsJson = nlohmann::json::array();
        for (const auto& wf : work->m_workflows)
        {
            if (!wf) continue;
            nlohmann::json wfJson;
            wfJson[ConstGameObjectSaveParamPref::kWorkflowName] = wf->m_name;
            wfJson[ConstGameObjectSaveParamPref::kWorkflowMode] = (wf->m_mode == EWorkFlowMode::Sequential)
	                                                                  ? ConstGameObjectSaveParamPref::kWorkflowModeSequential : ConstGameObjectSaveParamPref::kWorkflowModeParallel;
            nlohmann::json tasksJson = nlohmann::json::array();
            for (TriggerComponent* task : wf->m_tasks)
            {
                if (!task)
                {
                    tasksJson.push_back(nullptr);
                    continue;
                }
                auto it = triggerRefMap.find(task);
                if (it != triggerRefMap.end())
                {
                    nlohmann::json ref;
                    ref[ConstGameObjectSaveParamPref::kTaskRefGameObjectName] = it->second.first;
                    ref[ConstGameObjectSaveParamPref::kTaskRefTriggerIndex] = it->second.second;
                    tasksJson.push_back(std::move(ref));
                }
                else
                    tasksJson.push_back(nullptr);
            }
            wfJson[ConstGameObjectSaveParamPref::kWorkflowTasks] = std::move(tasksJson);
            workflowsJson.push_back(std::move(wfJson));
        }
        workJson[ConstGameObjectSaveParamPref::kWorkflows] = std::move(workflowsJson);
        worksJson.push_back(std::move(workJson));
    }
    return worksJson;
}

void WorkManager::LoadFromSceneJson(const nlohmann::json& sceneJson,
    const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    Clear();
    if (!sceneJson.contains(ConstGameObjectSaveParamPref::kWorks) || !sceneJson[ConstGameObjectSaveParamPref::kWorks].is_array())
        return;

    auto& factory = TriggerFactory::GetInstance();
    struct StartWorkPending { Work* work; size_t actionIndex; std::string workName; };
    std::vector<StartWorkPending> startWorkPendingList;

    for (const auto& workJson : sceneJson[ConstGameObjectSaveParamPref::kWorks])
    {
        if (!workJson.contains(ConstGameObjectSaveParamPref::kWorkName)) continue;
        std::string name = workJson[ConstGameObjectSaveParamPref::kWorkName].get<std::string>();
        auto work = std::make_unique<Work>(name);
        Work* workPtr = work.get();

        if (workJson.contains(ConstGameObjectSaveParamPref::kWorkStartCondition) && !workJson[ConstGameObjectSaveParamPref::kWorkStartCondition].is_null())
        {
            std::string condName = workJson[ConstGameObjectSaveParamPref::kWorkStartCondition].get<std::string>();
            work->m_startCondition = factory.CreateCondition(condName);
        }

        if (workJson.contains(ConstGameObjectSaveParamPref::kWorkRewardActions) && workJson[ConstGameObjectSaveParamPref::kWorkRewardActions].is_array())
        {
            size_t actionIndex = 0;
            for (const auto& actionJson : workJson[ConstGameObjectSaveParamPref::kWorkRewardActions])
            {
                if (!actionJson.contains(ConstGameObjectSaveParamPref::kRewardActionType)) continue;
                std::string actionType = actionJson[ConstGameObjectSaveParamPref::kRewardActionType].get<std::string>();
                if (auto action = factory.CreateAction(actionType))
                {
                    if (actionType == "AddSceneObjectToItemListReward" && actionJson.contains(ConstGameObjectSaveParamPref::kRewardActionGameObjectName))
                    {
                        auto* addSceneObjReward = dynamic_cast<AddSceneObjectToItemListReward*>(action.get());
                        if (addSceneObjReward)
                            addSceneObjReward->SetObjectName(actionJson[ConstGameObjectSaveParamPref::kRewardActionGameObjectName].get<std::string>());
                    }
                    if (actionType == "PlaceHeldItemReward")
                    {
                        auto* placeHeldReward = dynamic_cast<PlaceHeldItemReward*>(action.get());
                        if (placeHeldReward)
                        {
                            if (actionJson.contains(ConstGameObjectSaveParamPref::kRewardActionPlacePosition) &&
                                actionJson[ConstGameObjectSaveParamPref::kRewardActionPlacePosition].is_array() &&
                                actionJson[ConstGameObjectSaveParamPref::kRewardActionPlacePosition].size() >= 3)
                            {
                                const auto& arr = actionJson[ConstGameObjectSaveParamPref::kRewardActionPlacePosition];
                                placeHeldReward->SetPosition({
                                    arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>()
                                });
                            }
                            if (actionJson.contains(ConstGameObjectSaveParamPref::kRewardActionPlaceTargetObject))
                                placeHeldReward->SetTargetObjectName(actionJson[ConstGameObjectSaveParamPref::kRewardActionPlaceTargetObject].get<std::string>());
                        }
                    }
                    if (actionType == "PlaySoundAction")
                    {
                        auto* ps = dynamic_cast<PlaySoundReward*>(action.get());
                        if (ps)
                        {
                            if (actionJson.contains(ConstGameObjectSaveParamPref::kRewardActionSoundName))
                                ps->SetSoundName(actionJson[ConstGameObjectSaveParamPref::kRewardActionSoundName].get<std::string>());
                            if (actionJson.contains(ConstGameObjectSaveParamPref::kRewardActionSoundUse3d))
                                ps->SetUse3d(actionJson[ConstGameObjectSaveParamPref::kRewardActionSoundUse3d].get<bool>());
                        }
                    }
                    work->m_rewardActions.push_back(std::move(action));
                    if (actionType == "StartWorkReward" && actionJson.contains(ConstGameObjectSaveParamPref::kRewardActionWorkName))
                        startWorkPendingList.push_back({ workPtr, actionIndex, actionJson[ConstGameObjectSaveParamPref::kRewardActionWorkName].get<std::string>() });
                    ++actionIndex;
                }
            }
        }

        if (workJson.contains(ConstGameObjectSaveParamPref::kWorkflows) && workJson[ConstGameObjectSaveParamPref::kWorkflows].is_array())
        {
            for (const auto& wfJson : workJson[ConstGameObjectSaveParamPref::kWorkflows])
            {
                if (!wfJson.contains(ConstGameObjectSaveParamPref::kWorkflowName)) continue;
                std::string wfName = wfJson[ConstGameObjectSaveParamPref::kWorkflowName].get<std::string>();
                auto wf = std::make_unique<WorkFlow>(wfName);
                if (wfJson.contains(ConstGameObjectSaveParamPref::kWorkflowMode))
                {
                    std::string modeStr = wfJson[ConstGameObjectSaveParamPref::kWorkflowMode].get<std::string>();
                    wf->m_mode = (modeStr == ConstGameObjectSaveParamPref::kWorkflowModeParallel) ? EWorkFlowMode::Parallel : EWorkFlowMode::Sequential;
                }
                if (wfJson.contains(ConstGameObjectSaveParamPref::kWorkflowTasks) && wfJson[ConstGameObjectSaveParamPref::kWorkflowTasks].is_array())
                {
                    for (const auto& taskRef : wfJson[ConstGameObjectSaveParamPref::kWorkflowTasks])
                    {
                        if (taskRef.is_null())
                        {
                            wf->m_tasks.push_back(nullptr);
                            continue;
                        }
                        if (TriggerComponent* trigger = ResolveTaskRef(taskRef, gameObjects))
                            wf->m_tasks.push_back(trigger);
                        else
                            wf->m_tasks.push_back(nullptr);
                    }
                }
                work->m_workflows.push_back(std::move(wf));
            }
        }

        m_works.push_back(std::move(work));
    }

    for (const auto& pending : startWorkPendingList)
    {
        if (pending.actionIndex >= pending.work->m_rewardActions.size()) continue;
        auto* startWork = dynamic_cast<StartWorkReward*>(pending.work->m_rewardActions[pending.actionIndex].get());
        if (!startWork) continue;
        for (const auto& w : m_works)
        {
            if (w && w->m_name == pending.workName)
            {
                startWork->SetWork(w.get());
                break;
            }
        }
    }
}