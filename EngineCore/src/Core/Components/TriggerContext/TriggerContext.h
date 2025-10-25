#pragma once
#include <memory>

#include "Scene/GameObject/GameObject.h"

class TriggerContext
{
public:
    std::shared_ptr<GameObject> m_Owner;
    float m_DeltaTime = 0.0f;
};
