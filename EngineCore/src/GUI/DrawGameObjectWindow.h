#pragma once
#include "IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Scene/GameObject/Component/ComponentFactory.h"

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
            if (ImGui::Selectable(label.c_str(), s_SelectedObject == obj))
            {
                s_SelectedObject = obj;
            }
        }
        ImGui::EndChild();

        // === 右側：オブジェクトのTransform編集 ===
        ImGui::SameLine();

        bool is_selected_object_valid = false;
        if (s_SelectedObject)
        {
            // 現在のシーンリストに s_SelectedObject が含まれているかチェック
            for (const auto& obj : g_Scene->get_game_objects())
            {
                if (obj == s_SelectedObject)
                {
                    is_selected_object_valid = true;
                    break;
                }
            }
        }

        if (!is_selected_object_valid)
        {
            s_SelectedObject = nullptr;
        }

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
            
            // Components
            for (const auto& comp : s_SelectedObject->components)
            {
                if (!comp) continue;

                ImGui::PushID(comp.get());

                std::string componentType = comp->get_type(); 

                if (ImGui::CollapsingHeader(componentType.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::Button("Remove"))
                    {
                        // コンポーネントの削除
                        s_SelectedObject->RemoveComponent(comp.get());
                        ImGui::PopID();
                        break; // コンポーネントリストが変更されたのでループを抜ける
                    }
                    
                    comp->on_gui(); 
                }

                ImGui::PopID();
                ImGui::Separator();
            }
        }
        else
        {
            ImGui::Text("No object selected.");
        }
        ImGui::EndChild();

        // Componentの追加
        // Componentの追加
        if (s_SelectedObject) // s_SelectedObject は std::shared_ptr<GameObject>
        {
            ImGui::Separator();
            ImGui::Text("Add Component");

            // ファクトリから登録済みのコンポーネント名リストを取得
            const auto& component_map = ComponentFactory::get_mappings();

            // ドロップダウンリスト
            static std::string selected_component_type;
            if (component_map.empty())
            {
                ImGui::Text("No components registered.");
            }
            else
            {
                // 選択肢が空なら、最初の要素をデフォルトにする
                if (selected_component_type.empty()) {
                    selected_component_type = component_map.begin()->first;
                }

                if (ImGui::BeginCombo("Component Type", selected_component_type.c_str()))
                {
                    for (const auto& pair : component_map)
                    {
                        bool isSelected = (selected_component_type == pair.first);
                        if (ImGui::Selectable(pair.first.c_str(), isSelected))
                        {
                            selected_component_type = pair.first;
                        }
                        if (isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                if (ImGui::Button("Add"))
                {
                    // ファクトリに文字列名を渡してコンポーネントを
                    auto newComp = 
                        ComponentFactory::create(selected_component_type);

                    // 作成したコンポーネントを
                    if (newComp)
                    {
                        newComp->initialize(s_SelectedObject);
                        s_SelectedObject->components.push_back(std::move(newComp));
                    }
                }
            }
        }

        ImGui::End();
    }

private:
    std::shared_ptr<GameObject> s_SelectedObject = nullptr;
};
