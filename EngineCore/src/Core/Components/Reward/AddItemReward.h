#pragma once
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/Character/Player/Player.h"
#include <string>

/// <summary>
/// プレイヤーのインベントリにアイテムを追加する Reward。
/// </summary>
class AddItemReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        Player::GetInstance().AddItem(m_itemId);
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        constexpr size_t kBufferSize = 256;
        char buffer[kBufferSize];
        strncpy_s(buffer, m_itemId.c_str(), kBufferSize - 1);
        buffer[kBufferSize - 1] = '\0';
        if (ImGui::InputText("Item ID", buffer, kBufferSize))
        {
            m_itemId = buffer;
        }
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "AddItemReward";
    }

    const std::string& GetItemId() const { return m_itemId; }
    void SetItemId(const std::string& itemId) { m_itemId = itemId; }

private:
    std::string m_itemId;
};
