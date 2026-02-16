#include "TriggerComponent.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Scene/GameObject/GameObject.h"

void TriggerComponent::ResolvePendingWorkReferencesInScene(
    const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    for (const auto& obj : gameObjects)
    {
        if (!obj) continue;
        const auto triggers = obj->FindComponents<TriggerComponent>();
        for (TriggerComponent* trigger : triggers)
        {
            if (!trigger) continue;
            for (const auto& action : trigger->Actions)
            {
                auto* sw = dynamic_cast<StartWorkReward*>(action.get());
                if (sw) sw->ResolvePendingWork();
            }
        }
    }
}
