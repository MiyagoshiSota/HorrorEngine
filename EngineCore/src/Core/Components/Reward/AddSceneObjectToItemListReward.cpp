#include "AddSceneObjectToItemListReward.h"
#include "Core/App.h"

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif

#ifndef BUILD_STANDALONE
void AddSceneObjectToItemListReward::DrawInspectorUI()
{
    const char* preview = m_objectName.empty() ? "(Select object...)" : m_objectName.c_str();
    if (ImGui::BeginCombo("Scene Object", preview))
    {
        if (g_Scene)
        {
            for (const auto& obj : g_Scene->GetGameObjects())
            {
                if (!obj) continue;
                const std::string& name = obj->GetName();
                if (name.empty()) continue;
                const bool isSelected = (m_objectName == name);
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    m_objectName = name;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        else
        {
            ImGui::TextDisabled("(No scene loaded)");
        }
        ImGui::EndCombo();
    }
}
#endif
