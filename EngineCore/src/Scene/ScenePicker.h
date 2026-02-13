#pragma once

#include <reactphysics3d/reactphysics3d.h>

class GameObject;

/// <summary>
/// 毎フレーム、マウス位置からカメラレイを飛ばし物理ワールドでレイキャストする。
/// PlayMode の Scene->Update() の前に Update() を呼び、クリックされた GameObject を取得する。
/// </summary>
class ScenePicker
{
public:
    static ScenePicker& GetInstance();

    ScenePicker(const ScenePicker&) = delete;
    ScenePicker& operator=(const ScenePicker&) = delete;

    /// <summary>
    /// カメラとマウスからレイを構築し、物理レイキャストを実行する。
    /// g_Scene と GetSceneCamera(), GetPhysicsWorld() が有効なときに呼ぶこと。
    /// </summary>
    void Update();

    /// <summary>このフレームでレイが当たった GameObject（最も手前）。当たっていなければ nullptr。</summary>
    GameObject* GetPickedObject() const { return m_pickedGameObject; }

    /// <summary>このフレームで左クリックが押されたか（IsMousePressed(0)）。</summary>
    bool DidClickThisFrame() const { return m_didClickThisFrame; }

private:
    ScenePicker() = default;

    class PickerRaycastCallback : public reactphysics3d::RaycastCallback
    {
    public:
        reactphysics3d::decimal notifyRaycastHit(const reactphysics3d::RaycastInfo& raycastInfo) override;

        GameObject* GetHitGameObject() const { return m_hitGameObject; }

    private:
        reactphysics3d::decimal m_closestFraction = reactphysics3d::decimal(1.0);
        GameObject* m_hitGameObject = nullptr;
    };

    GameObject* m_pickedGameObject = nullptr;
    bool m_didClickThisFrame = false;
};
