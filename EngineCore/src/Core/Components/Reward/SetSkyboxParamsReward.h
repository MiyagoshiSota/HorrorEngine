#pragma once
#include "IReward.h"
#include "Core/App.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Scene/Skybox/SkyboxManager.h"
#include <DirectXMath.h>

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE

/// Skybox の定数（Intensity, Tint）を変更する Reward。
class SetSkyboxParamsReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        (void)context;
        if (!g_Scene)
            return;

        auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
        if (!defaultScene)
            return;

        auto skyboxManager = defaultScene->GetSkyboxManager();
        if (!skyboxManager || !skyboxManager->IsValid())
            return;

        skyboxManager->SetIntensity(m_intensity);
        skyboxManager->SetTint(m_tint);
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        ImGui::DragFloat("Intensity", &m_intensity, 0.01f, 0.0f, 10.0f, "%.2f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Skybox の明るさ倍率（1.0 = 変化なし）");

        ImGui::ColorEdit3("Tint", &m_tint.x);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Skybox に掛ける色ティント（1,1,1 = 変化なし）");
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override { return "SetSkyboxParamsReward"; }

private:
    float m_intensity = 1.0f;
    DirectX::XMFLOAT3 m_tint = { 1.0f, 1.0f, 1.0f };
};
