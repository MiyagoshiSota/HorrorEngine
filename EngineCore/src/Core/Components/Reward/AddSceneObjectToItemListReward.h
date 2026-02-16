#pragma once
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/Character/Player/Player.h"
#include <string>

/// <summary>
/// シーン内のオブジェクト（GameObject名）をプレイヤーのインベントリ（ItemList）に追加する Reward。
/// シーン内のオブジェクトは一意のため、プルダウンで選択する。
/// </summary>
class AddSceneObjectToItemListReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        if (!m_objectName.empty())
        {
            Player::GetInstance().AddItem(m_objectName);
        }
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override;
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "AddSceneObjectToItemListReward";
    }

    const std::string& GetObjectName() const { return m_objectName; }
    void SetObjectName(const std::string& name) { m_objectName = name; }

private:
    std::string m_objectName;
};
