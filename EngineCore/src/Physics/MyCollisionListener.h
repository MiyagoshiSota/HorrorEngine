#pragma once
#include <reactphysics3d/reactphysics3d.h>

#include "Component/Rigidbody.h"
#include "Scene/GameObject/GameObject.h"

namespace rp3d = reactphysics3d;

class MyCollisionListener : public rp3d::EventListener
{
public:
    // 物理ワールドで接触イベントが発生したときに自動的に呼び出される
    void onContact(const CollisionCallback::CallbackData& callbackData) override
    {
        // このフレームで発生した全ての接触ペアに対してループ
        for (reactphysics3d::uint32 p = 0; p < callbackData.getNbContactPairs(); p++)
        {
            CollisionCallback::ContactPair contactPair = callbackData.getContactPair(p);

            // 衝突した2つのGameObjectを取得
            GameObject* go1 = static_cast<GameObject*>(contactPair.getBody1()->getUserData());
            GameObject* go2 = static_cast<GameObject*>(contactPair.getBody2()->getUserData());

            // どちらかのポインタが無効なら、このペアの処理はスキップ
            if (go1 == nullptr || go2 == nullptr) continue;

            // 各GameObjectからRigidbodyコンポーネントを取得
            auto rb1 = go1->FindComponent<Rigidbody>();
            auto rb2 = go2->FindComponent<Rigidbody>();

            // どちらかがRigidbodyを持っていなければスキップ
            if (rb1 == nullptr || rb2 == nullptr) continue;

            // イベントの種類に応じて、各コンポーネントに通知
            switch (contactPair.getEventType())
            {
            case CollisionCallback::ContactPair::EventType::ContactStart:
                {
                    rb1->OnCollisionEnter(go2);
                    rb2->OnCollisionEnter(go1);
                    break;
                }
            case CollisionCallback::ContactPair::EventType::ContactExit:
                {
                    rb1->OnCollisionExit(go2);
                    rb2->OnCollisionExit(go1);
                    break;
                }
            case CollisionCallback::ContactPair::EventType::ContactStay:
                {
                    break;
                }
            }
        }
    }
};