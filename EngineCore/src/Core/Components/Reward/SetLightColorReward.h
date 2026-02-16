#pragma once
#include "IReward.h"
#include "Core/App.h"
#include "Renderer/Light/Light.h"
#include "Renderer/Light/LightingManager.h"
#include "Scene/ISceneBase.h"
#include <DirectXMath.h>
#include <string>

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE

/// LightingManager の指定インデックスのライトの色を変更する Reward。
/// ライトタイプ（Directional/Point/Spot）とインデックスで対象を指定する。
class SetLightColorReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        (void)context;
        if (!g_Scene)
            return;

        auto lightManager = g_Scene->GetLightingManager();
        if (!lightManager)
            return;

        std::shared_ptr<Light> targetLight = FindTargetLight(*lightManager);
        if (!targetLight)
            return;

        targetLight->SetColor(m_color);
        if (m_overrideIntensity)
            targetLight->Intensity = m_intensity;
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        const char* typeNames[] = { "Directional", "Point", "Spot" };
        int typeIdx = static_cast<int>(m_lightType);
        if (ImGui::Combo("Light Type", &typeIdx, typeNames, 3))
            m_lightType = static_cast<LightType>(typeIdx);

        ImGui::DragInt("Light Index", &m_lightIndex, 1.0f, 0, 31);

        ImGui::ColorEdit3("Color", &m_color.x);

        ImGui::Checkbox("Override Intensity", &m_overrideIntensity);
        if (m_overrideIntensity)
            ImGui::DragFloat("Intensity", &m_intensity, 0.01f, 0.0f, 100.0f);
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override { return "SetLightColorReward"; }

private:
    std::shared_ptr<Light> FindTargetLight(LightingManager& manager) const
    {
        switch (m_lightType)
        {
        case LightType::Directional:
        {
            const auto& lights = manager.GetDirectionalLights();
            if (m_lightIndex >= 0 && static_cast<size_t>(m_lightIndex) < lights.size())
                return lights[static_cast<size_t>(m_lightIndex)];
            break;
        }
        case LightType::Point:
        {
            const auto& lights = manager.GetPointLights();
            if (m_lightIndex >= 0 && static_cast<size_t>(m_lightIndex) < lights.size())
                return lights[static_cast<size_t>(m_lightIndex)];
            break;
        }
        case LightType::Spot:
        {
            const auto& lights = manager.GetSpotLights();
            if (m_lightIndex >= 0 && static_cast<size_t>(m_lightIndex) < lights.size())
                return lights[static_cast<size_t>(m_lightIndex)];
            break;
        }
        default:
            break;
        }
        return nullptr;
    }

    LightType m_lightType = LightType::Directional;
    int m_lightIndex = 0;
    DirectX::XMFLOAT3 m_color = { 1.0f, 1.0f, 1.0f };
    bool m_overrideIntensity = false;
    float m_intensity = 1.0f;
};
