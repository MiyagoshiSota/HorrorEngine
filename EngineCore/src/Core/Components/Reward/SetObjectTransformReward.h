#pragma once
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/GameObject/Find/GameObjectFInder.h"
#include "Scene/GameObject/GameObject.h"
#include <DirectXMath.h>
#include <string>

/// <summary>
/// 名前で指定した GameObject の Position / Rotation / Scale を設定する Reward。
/// </summary>
class SetObjectTransformReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        auto target = GameObjectFinder::FindGameObjectsByName(m_targetName);
        if (target == nullptr)
            return;

        if (m_applyPosition)
            target->SetPosition(m_position.x, m_position.y, m_position.z);
        if (m_applyRotation)
            target->SetRotation(m_rotation.x, m_rotation.y, m_rotation.z);
        if (m_applyScale)
            target->SetScale(m_scale.x, m_scale.y, m_scale.z);
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        constexpr size_t kNameSize = 256;
        char nameBuf[kNameSize];
        strncpy_s(nameBuf, m_targetName.c_str(), kNameSize - 1);
        nameBuf[kNameSize - 1] = '\0';
        if (ImGui::InputText("Target Name", nameBuf, kNameSize))
            m_targetName = nameBuf;

        ImGui::Checkbox("Apply Position", &m_applyPosition);
        if (m_applyPosition)
            ImGui::DragFloat3("Position", &m_position.x, 0.1f);

        ImGui::Checkbox("Apply Rotation", &m_applyRotation);
        if (m_applyRotation)
            ImGui::DragFloat3("Rotation (deg)", &m_rotation.x, 1.0f);

        ImGui::Checkbox("Apply Scale", &m_applyScale);
        if (m_applyScale)
            ImGui::DragFloat3("Scale", &m_scale.x, 0.01f);
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "SetObjectTransformReward";
    }

private:
    std::string m_targetName;
    bool m_applyPosition = true;
    bool m_applyRotation = false;
    bool m_applyScale = false;
    DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_rotation = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };
};
