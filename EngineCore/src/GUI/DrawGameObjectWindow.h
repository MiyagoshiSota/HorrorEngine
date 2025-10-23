#pragma once
#include "IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"

class DrawGameObjectWindow : public IDrawWindow
{
public:
    void draw() override
    {
        ImGui::Begin("Game Object Window");

        // === 左側：オブジェクト一覧 ===
        ImGui::BeginChild("Object List", ImVec2(200, 0), true);
        for (const auto& obj : g_Scene->get_game_objects())
        {
            std::string label = obj->get_name();
            if (ImGui::Selectable(label.c_str(), s_SelectedObject == obj.get()))
            {
                s_SelectedObject = obj.get();
            }
        }
        ImGui::EndChild();

        // === 右側：オブジェクトのTransform編集 ===
        ImGui::SameLine();

        ImGui::BeginChild("Object Properties", ImVec2(0, 0), true);
        if (s_SelectedObject)
        {
            ImGui::Text("Editing: %s", s_SelectedObject->get_name().c_str());
            ImGui::Separator();

            // Position
            {
                auto pos = s_SelectedObject->get_position();
                if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                {
                    s_SelectedObject->set_position(pos.x,pos.y,pos.z);
                }
            }

            // Rotation
            {
                auto rot = s_SelectedObject->get_rotation();
                if (ImGui::DragFloat3("Rotation", &rot.x, 0.5f))
                {
                    s_SelectedObject->set_rotation(rot.x,rot.y,rot.z);
                }
            }

            // Scale
            {
                auto scale = s_SelectedObject->get_scale();
                if (ImGui::DragFloat3("Scale", &scale.x, 0.1f))
                {
                    s_SelectedObject->set_scale(scale.x,scale.y,scale.z);
                }
            }
        }
        else
        {
            ImGui::Text("No object selected.");
        }
        ImGui::EndChild();

        ImGui::End();
    }

private:
    GameObject* s_SelectedObject = nullptr;
};
