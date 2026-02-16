#pragma once
#include <memory>
#include <string>
#include <vector>

#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Pass/PostProcess/Manager/PostProcessManager.h"
#include "Renderer/Target/ITargetBase.h"
#include "Scene/Default/Scene/DefaultScene.h"

class DrawPostProcessPresetWindow : public IDrawWindow
{
public:
    DrawPostProcessPresetWindow() = default;

    void draw() override
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f, 100.0f), ImVec2(FLT_MAX, FLT_MAX));

        auto scene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
        std::shared_ptr<PostProcessManager> ppManager = (scene != nullptr) ? scene->GetPostProcessManager() : nullptr;

        if (!ImGui::Begin("Post Process Preset Window", &m_isVisible))
        {
            if (ppManager != nullptr)
                ppManager->SetCapturePassOutputsForDebug(false);
            ImGui::End();
            return;
        }

        if (ppManager != nullptr)
            ppManager->SetCapturePassOutputsForDebug(true);

        if (ImGui::BeginCombo("Preset", m_currentPresetName.c_str()))
        {
            if (ppManager != nullptr)
            {
                for (const auto& presetName : ppManager->GetPresetNames())
                {
                    const bool isSelected = (m_currentPresetName == presetName);
                    if (ImGui::Selectable(presetName.c_str(), isSelected))
                    {
                        m_currentPresetName = presetName;
                        ppManager->BlendToPreset(m_currentPresetName, 1.0f);
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Pass Output Textures");

        if (ppManager != nullptr)
        {
            const std::vector<std::string>& order = ppManager->GetCurrentPresetOrder();
            const float maxPreviewSize = 280.0f;

            for (const std::string& passName : order)
            {
                std::shared_ptr<ITargetBase> rt = ppManager->GetDebugPassOutput(passName);
                if (rt && rt->GetSRVHandle() && rt->GetResource())
                {
                    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = g_Engine->GetImGuiSrvForTexture(rt->GetResource());
                    if (gpuHandle.ptr != 0)
                    {
                        const float w = static_cast<float>(rt->GetWidth());
                        const float h = static_cast<float>(rt->GetHeight());
                        const float aspect = (h > 0.0f) ? (w / h) : 1.0f;
                        float displayW = w * 0.5f;
                        float displayH = h * 0.5f;
                        if (displayW > maxPreviewSize || displayH > maxPreviewSize)
                        {
                            if (aspect >= 1.0f)
                            {
                                displayW = maxPreviewSize;
                                displayH = maxPreviewSize / aspect;
                            }
                            else
                            {
                                displayH = maxPreviewSize;
                                displayW = maxPreviewSize * aspect;
                            }
                        }
                        ImGui::Text("%s (%u x %u)", passName.c_str(), rt->GetWidth(), rt->GetHeight());
                        ImGui::Image((ImTextureID)(uintptr_t)gpuHandle.ptr, ImVec2(displayW, displayH));
                    }
                    else
                        ImGui::TextDisabled("%s (ImGui SRV failed)", passName.c_str());
                }
                else
                    ImGui::TextDisabled("%s (no capture)", passName.c_str());
            }

            if (order.empty())
                ImGui::TextDisabled("(No passes in preset)");
        }
        else
            ImGui::TextDisabled("(PostProcessManager not available)");

        ImGui::End();
    }

private:
    std::string m_currentPresetName = "Normal";
};
