#pragma once
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/Character/Player/Player.h"
#include "Scene/GameObject/Find/GameObjectFInder.h"
#include <DirectXMath.h>
#include <string>

/// <summary>
/// 手持ちアイテムを指定位置に置く Reward。
/// オブジェクトはシーンに残り、インベントリから削除される。
/// 何も持っていない場合は何もしない。
/// </summary>
class PlaceHeldItemReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        float x = m_position.x;
        float y = m_position.y;
        float z = m_position.z;

        if (!m_targetObjectName.empty())
        {
            auto target = GameObjectFinder::FindGameObjectsByName(m_targetObjectName);
            if (target)
            {
                const auto pos = target->GetPosition();
                x = pos.x;
                y = pos.y;
                z = pos.z;
            }
        }

        Player::GetInstance().PlaceHeldItemAt(x, y, z);
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override;
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "PlaceHeldItemReward";
    }

    const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
    void SetPosition(const DirectX::XMFLOAT3& pos) { m_position = pos; }
    const std::string& GetTargetObjectName() const { return m_targetObjectName; }
    void SetTargetObjectName(const std::string& name) { m_targetObjectName = name; }

private:
    DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
    std::string m_targetObjectName;
};
