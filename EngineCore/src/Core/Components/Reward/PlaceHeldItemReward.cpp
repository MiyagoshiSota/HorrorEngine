#include "PlaceHeldItemReward.h"
#include "Core/App.h"

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif

#ifndef BUILD_STANDALONE
void PlaceHeldItemReward::DrawInspectorUI()
{
    const char* preview = m_targetObjectName.empty() ? "(Use Position)" : m_targetObjectName.c_str();
    if (ImGui::BeginCombo("Place At (Object)", preview))
    {
        if (ImGui::Selectable("(Use Position)", m_targetObjectName.empty()))
        {
            m_targetObjectName.clear();
        }
        if (m_targetObjectName.empty())
            ImGui::SetItemDefaultFocus();
        if (g_Scene)
        {
            for (const auto& obj : g_Scene->GetGameObjects())
            {
                if (!obj) continue;
                const std::string& name = obj->GetName();
                if (name.empty()) continue;
                const bool isSelected = (m_targetObjectName == name);
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    m_targetObjectName = name;
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (m_targetObjectName.empty())
    {
        ImGui::DragFloat3("Position", &m_position.x, 0.1f);
    }
}
#endif
