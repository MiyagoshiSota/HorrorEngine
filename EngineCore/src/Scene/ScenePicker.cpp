#include "ScenePicker.h"
#include "Core/App.h"
#include "Input/InputDevice.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/ISceneBase.h"
#include <DirectXMath.h>

namespace
{
    constexpr float kRayLength = 10000.0f;
}

ScenePicker& ScenePicker::GetInstance()
{
    static ScenePicker instance;
    return instance;
}

reactphysics3d::decimal ScenePicker::PickerRaycastCallback::notifyRaycastHit(const reactphysics3d::RaycastInfo& raycastInfo)
{
    if (raycastInfo.body == nullptr)
        return reactphysics3d::decimal(1.0);

    if (raycastInfo.hitFraction < m_closestFraction)
    {
        m_closestFraction = raycastInfo.hitFraction;
        m_hitGameObject = static_cast<GameObject*>(raycastInfo.body->getUserData());
    }
    return raycastInfo.hitFraction;
}

void ScenePicker::Update()
{
    m_pickedGameObject = nullptr;
    m_didClickThisFrame = InputDevice::GetInstance().IsMousePressed(0);

    if (g_Scene == nullptr)
        return;

    auto camera = g_Scene->GetSceneCamera();
    auto* physicsWorld = g_Scene->GetPhysicsWorld();
    if (camera == nullptr || physicsWorld == nullptr)
        return;

    const DirectX::XMFLOAT2 mousePos = InputDevice::GetInstance().GetMousePosition();
    const float width = static_cast<float>(kWindowWidth);
    const float height = static_cast<float>(kWindowHeight);
    const float ndcX = (mousePos.x / width) * 2.0f - 1.0f;
    const float ndcY = -((mousePos.y / height) * 2.0f - 1.0f);

    const DirectX::XMMATRIX view = camera->GetViewMatrix();
    const DirectX::XMMATRIX proj = camera->GetProjectionMatrix();
    const DirectX::XMMATRIX viewProj = view * proj;
    const DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(nullptr, viewProj);

    const DirectX::XMVECTOR ndcNear = DirectX::XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);
    const DirectX::XMVECTOR worldNear = DirectX::XMVector3TransformCoord(ndcNear, invViewProj);
    const DirectX::XMVECTOR eye = camera->GetEyePos();
    DirectX::XMVECTOR dir = DirectX::XMVectorSubtract(worldNear, eye);
    dir = DirectX::XMVector3Normalize(dir);

    DirectX::XMFLOAT3 eyeF3;
    DirectX::XMStoreFloat3(&eyeF3, eye);
    DirectX::XMFLOAT3 dirF3;
    DirectX::XMStoreFloat3(&dirF3, dir);

    const reactphysics3d::Vector3 origin(static_cast<reactphysics3d::decimal>(eyeF3.x),
                                         static_cast<reactphysics3d::decimal>(eyeF3.y),
                                         static_cast<reactphysics3d::decimal>(eyeF3.z));
    const reactphysics3d::Vector3 end(
        static_cast<reactphysics3d::decimal>(eyeF3.x + dirF3.x * kRayLength),
        static_cast<reactphysics3d::decimal>(eyeF3.y + dirF3.y * kRayLength),
        static_cast<reactphysics3d::decimal>(eyeF3.z + dirF3.z * kRayLength));

    const reactphysics3d::Ray ray(origin, end, reactphysics3d::decimal(1.0));

    PickerRaycastCallback callback;
    physicsWorld->raycast(ray, &callback);

    m_pickedGameObject = callback.GetHitGameObject();
}
