#pragma once
#include "IReward.h"
#include "Core/App.h"
#include "Renderer/Pass/PostProcess/Manager/PostProcessManager.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include <memory>
#include <string>

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE

/// PostProcess のプリセットへブレンドする Reward。
/// duration が 0 の場合は即座に切り替え、0 より大きい場合は指定秒数でブレンドする。
class BlendPostProcessPresetReward : public IReward
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

        auto ppManager = defaultScene->GetPostProcessManager();
        if (!ppManager)
            return;

        ppManager->BlendToPreset(m_presetName, m_duration);
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        constexpr size_t kNameSize = 256;
        char nameBuf[kNameSize];
        strncpy_s(nameBuf, m_presetName.c_str(), kNameSize - 1);
        nameBuf[kNameSize - 1] = '\0';
        if (ImGui::InputText("Preset Name", nameBuf, kNameSize))
            m_presetName = nameBuf;

        ImGui::DragFloat("Blend Duration (sec)", &m_duration, 0.1f, 0.0f, 10.0f, "%.2f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("0 = instant switch, >0 = blend over N seconds");

        // プリセット一覧を補助表示（DefaultScene が利用可能な場合）
        auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
        if (defaultScene)
        {
            auto ppManager = defaultScene->GetPostProcessManager();
            if (ppManager)
            {
                const auto names = ppManager->GetPresetNames();
                if (ImGui::BeginCombo("##PresetHelper", "(Select preset)"))
                {
                    for (const auto& name : names)
                    {
                        bool selected = (m_presetName == name);
                        if (ImGui::Selectable(name.c_str(), selected))
                            m_presetName = name;
                        if (selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Quick select from loaded presets");
            }
        }
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override { return "BlendPostProcessPresetReward"; }

private:
    std::string m_presetName = "Normal";
    float m_duration = 1.0f;
};
