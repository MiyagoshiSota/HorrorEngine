#pragma once
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/TriggerContext/TriggerContext.h"

#include "Physics/Component/Rigidbody.h"

class OnGameObjectEnterCondition : public ITriggerCondition
{
public:
    bool Check(const TriggerContext& context) override
    {
        // Rigidbodyコンポーネントを取得
        auto rbComponent = context.m_Owner->find_component<Rigidbody>();
        if (rbComponent == nullptr) {
            // Rigidbodyがなければ衝突判定はできない
            return false;
        }
        
        return rbComponent->IsColliding();
    }

    void DrawInspectorUI() override
    {
        // インスペクターに表示する設定項目があればここに実装
        // 例えば、トリガー領域のサイズやその他のパラメータを設定できるようにする
    }

    std::string GetName() const override
    {
        return "OnGameObjectEnterCondition";
    };
};
