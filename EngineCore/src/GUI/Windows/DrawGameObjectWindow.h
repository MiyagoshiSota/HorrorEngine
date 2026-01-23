    #pragma once
    #include "GUI/Core/IDrawWindow.h"
    #include "imgui.h"
    #include "Core/App.h"
    #include "Scene/GameObject/Component/ComponentFactory.h"

    class DrawGameObjectWindow : public IDrawWindow
    {
    public:
        void draw() override
        {
            // 最小サイズ制限を設定（内部に複数のChildウィンドウがあるため）
            ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 300.0f), ImVec2(FLT_MAX, FLT_MAX));

            if (!ImGui::Begin("Game Object Window", &m_isVisible))
            {
                ImGui::End();
                return;
            }

            // === 左側：オブジェクト一覧 ===
            ImGui::BeginChild("Object List", ImVec2(200, 100), true);
            for (const auto& obj : g_Scene->GetGameObjects())
            {
                std::string label = obj->GetName();
                if (ImGui::Selectable(label.c_str(), s_SelectedObject == obj))
                {
                    s_SelectedObject = obj;
                }
            }
            ImGui::EndChild();

            // === 右側：オブジェクトのTransform編集 ===
            ImGui::SameLine();

            bool isSelectedObjectValid = false;
            if (s_SelectedObject)
            {
                // 現在のシーンリストに s_SelectedObject が含まれているかチェック
                for (const auto& obj : g_Scene->GetGameObjects())
                {
                    if (obj == s_SelectedObject)
                    {
                        isSelectedObjectValid = true;
                        break;
                    }
                }
            }

            if (!isSelectedObjectValid)
            {
                s_SelectedObject = nullptr;
            }

            ImGui::BeginChild("Object Properties", ImVec2(400, 100), true);
            if (s_SelectedObject)
            {
                ImGui::Text("Editing: %s", s_SelectedObject->GetName().c_str());
                ImGui::Separator();

                // Position
                {
                    auto pos = s_SelectedObject->GetPosition();
                    if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
                    {
                        s_SelectedObject->SetPosition(pos.x,pos.y,pos.z);
                    }
                }

                // Rotation
                {
                    auto rot = s_SelectedObject->GetRotation();
                    if (ImGui::DragFloat3("Rotation", &rot.x, 0.5f))
                    {
                        s_SelectedObject->SetRotation(rot.x,rot.y,rot.z);
                    }
                }

                // Scale
                {
                    auto scale = s_SelectedObject->GetScale();
                    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f))
                    {
                        s_SelectedObject->SetScale(scale.x,scale.y,scale.z);
                    }
                }
                
                // Components
                for (const auto& comp : s_SelectedObject->components)
                {
                    if (!comp) continue;

                    ImGui::PushID(comp.get());

                    std::string componentType = comp->GetType(); 

                    if (ImGui::CollapsingHeader(componentType.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (ImGui::Button("Remove"))
                        {
                            // コンポーネントの削除
                            s_SelectedObject->RemoveComponent(comp.get());
                            ImGui::PopID();
                            break; // コンポーネントリストが変更されたのでループを抜ける
                        }
                        
                        comp->OnGui(); 
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

            // Lightの一覧
            ImGui::BeginChild("Light List", ImVec2(200, 100), true);
            for (const auto& light : g_Scene->GetLightingManager()->GetLights())
            {
                std::string label = light->GetTypeString() + " Light";
                if (ImGui::Selectable(label.c_str(), s_SelectedLight == light))
                {
                    s_SelectedLight = light;
                }
            }
            ImGui::EndChild();

            // === Lightの編集 ===
            ImGui::SameLine();
            ImGui::BeginChild("Light Properties", ImVec2(400, 100), true);
            // 以下に追記
            if (s_SelectedLight)
            {
                ImGui::Text("Editing: %s Light", s_SelectedLight->GetTypeString().c_str());
                ImGui::Separator();
                
                // Color
                {
                    auto color = s_SelectedLight->Color;
                    float colorArr[3] = { color.x, color.y, color.z };
                    if (ImGui::ColorEdit3("Color", colorArr))
                    {
                        s_SelectedLight->SetColor(DirectX::XMFLOAT3(colorArr[0], colorArr[1], colorArr[2]));
                    }
                }

                // Position
                if (s_SelectedLight->Type == LightType::Point)
                {
                    auto pointLight = std::dynamic_pointer_cast<PointLight>(s_SelectedLight);
                    if (pointLight)
                    {
                        auto position = pointLight->Position;
                        if (ImGui::DragFloat3("Position", &position.x, 0.1f))
                        {
                            pointLight->SetPosition(position.x, position.y, position.z);
                        }

                        float range = pointLight->Range;
                        if (ImGui::DragFloat("Range", &range, 1.0f, 0.0f, 10000.0f))
                        {
                            pointLight->Range = range;
                        }

                        float attenuation = pointLight->Attenuation;
                        if (ImGui::DragFloat("Attenuation", &attenuation, 0.001f, 0.0f, 1.0f))
                        {
                            pointLight->Attenuation = attenuation;
                        }
                    }
                }

				// DirectionalLightの編集
                if (s_SelectedLight->Type == LightType::Directional)
                {
                    auto dirLight = std::dynamic_pointer_cast<DirectionalLight>(s_SelectedLight);
                    if (dirLight)
                    {
                        auto direction = dirLight->Direction;
                        if (ImGui::DragFloat3("Direction", &direction.x, 0.1f))
                        {
                            dirLight->Direction = direction;
                        }
                    }
				}

                // SpotLightの編集
                if (s_SelectedLight->Type == LightType::Spot)
                {
                    auto spotLight = std::dynamic_pointer_cast<SpotLight>(s_SelectedLight);
                    if (spotLight)
                    {
                        auto position = spotLight->Position;
                        if (ImGui::DragFloat3("Position", &position.x, 0.1f))
                        {
                            spotLight->SetPosition(position.x, position.y, position.z);
                        }

                        auto direction = spotLight->Direction;
                        if (ImGui::DragFloat3("Direction", &direction.x, 0.1f))
                        {
                            spotLight->Direction = direction;
                        }

                        float innerAngle = spotLight->InnerAngle;
                        if (ImGui::DragFloat("Inner Angle", &innerAngle, 0.01f, 0.0f, 3.14f))
                        {
                            spotLight->InnerAngle = innerAngle;
                        }

                        float outerAngle = spotLight->OuterAngle;
                        if (ImGui::DragFloat("Outer Angle", &outerAngle, 0.01f, 0.0f, 3.14f))
                        {
                            spotLight->OuterAngle = outerAngle;
                        }

                        float range = spotLight->Range;
                        if (ImGui::DragFloat("Range", &range, 1.0f, 0.0f, 10000.0f))
                        {
                            spotLight->Range = range;
                        }

                        float attenuation = spotLight->Attenuation;
                        if (ImGui::DragFloat("Attenuation", &attenuation, 0.001f, 0.0f, 1.0f))
                        {
                            spotLight->Attenuation = attenuation;
                        }
                    }
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
                static std::string selectedComponentType;
                if (component_map.empty())
                {
                    ImGui::Text("No components registered.");
                }
                else
                {
                    // 選択肢が空なら、最初の要素をデフォルトにする
                    if (selectedComponentType.empty()) {
                        selectedComponentType = component_map.begin()->first;
                    }

                    if (ImGui::BeginCombo("Component Type", selectedComponentType.c_str()))
                    {
                        for (const auto& pair : component_map)
                        {
                            bool isSelected = (selectedComponentType == pair.first);
                            if (ImGui::Selectable(pair.first.c_str(), isSelected))
                            {
                                selectedComponentType = pair.first;
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
                            ComponentFactory::create(selectedComponentType);

                        // 作成したコンポーネントを
                        if (newComp)
                        {
                            newComp->Initialize(s_SelectedObject);
                            s_SelectedObject->components.push_back(std::move(newComp));
                        }
                    }
                }
            }

            ImGui::End();
        }

    private:
        std::shared_ptr<GameObject> s_SelectedObject = nullptr;
        std::shared_ptr<Light> s_SelectedLight = nullptr;
    };
