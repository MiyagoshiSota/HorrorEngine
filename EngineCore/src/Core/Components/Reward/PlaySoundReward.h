#pragma once
#include "IReward.h"
#include "Core/App.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/ISceneBase.h"
#include <string>

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE

/// 指定のサウンドを再生する Reward。2D/3D を選択可能。
class PlaySoundReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        (void)context;
        if (!g_Scene || m_soundName.empty())
            return;
        const auto audioManager = g_Scene->GetAudioManager();
        if (!audioManager)
            return;
        if (m_use3d)
            audioManager->PlaySfx3d(m_soundName);
        else
            audioManager->PlaySfx(m_soundName);
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        constexpr size_t kBufferSize = 256;
        char buffer[kBufferSize];
        strncpy_s(buffer, m_soundName.c_str(), kBufferSize - 1);
        buffer[kBufferSize - 1] = '\0';
        if (ImGui::InputText("Sound Name", buffer, kBufferSize))
            m_soundName = buffer;
        ImGui::Checkbox("Use 3D Sound", &m_use3d);
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override { return "PlaySoundAction"; }

    const std::string& GetSoundName() const { return m_soundName; }
    void SetSoundName(const std::string& soundName) { m_soundName = soundName; }
    bool GetUse3d() const { return m_use3d; }
    void SetUse3d(bool use3d) { m_use3d = use3d; }

private:
    std::string m_soundName;
    bool m_use3d = false;
};
