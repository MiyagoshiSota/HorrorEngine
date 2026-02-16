#pragma once
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/ScenePicker.h"
#include "Scene/Character/Player/Player.h"
#include <nlohmann/json.hpp>
#include <string>

/// <summary>
/// 指定アイテムを所持した状態で、このトリガーが付いた GameObject を左クリックしたときに true を返す Condition。
/// </summary>
class ClickWithItemCondition : public ITriggerCondition
{
public:
    bool Check(const TriggerContext& context) override
    {
        if (context.m_Owner == nullptr)
            return false;
        if (!Player::GetInstance().HasItem(m_requiredItemId))
            return false;
        return ScenePicker::GetInstance().DidClickThisFrame() &&
               ScenePicker::GetInstance().GetPickedObject() == context.m_Owner.get();
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        constexpr size_t kBufferSize = 256;
        char buffer[kBufferSize];
        strncpy_s(buffer, m_requiredItemId.c_str(), kBufferSize - 1);
        buffer[kBufferSize - 1] = '\0';
        if (ImGui::InputText("Required Item ID", buffer, kBufferSize))
        {
            m_requiredItemId = buffer;
        }
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "ClickWithItemCondition";
    }

    void Serialize(nlohmann::json& j) const override
    {
        j["requiredItemId"] = m_requiredItemId;
    }
    void Deserialize(const nlohmann::json& j) override
    {
        if (j.contains("requiredItemId"))
            m_requiredItemId = j["requiredItemId"].get<std::string>();
    }

    const std::string& GetRequiredItemId() const { return m_requiredItemId; }
    void SetRequiredItemId(const std::string& id) { m_requiredItemId = id; }

private:
    std::string m_requiredItemId;
};
