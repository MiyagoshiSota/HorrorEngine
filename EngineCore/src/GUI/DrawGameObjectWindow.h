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
        
        for (const auto& obj : g_Scene->get_game_objects())
        {
            // オブジェクトのIDや名前を使って、ユニークなラベルを作成
            std::string label = obj->get_name();
        
            // Selectableを使うと、選択状態を管理できる
            if (ImGui::Selectable(label.c_str(), s_SelectedObject == obj.get()))
            {
                s_SelectedObject = obj.get();
            }
        }
        
        ImGui::End();
    }

private:
    GameObject* s_SelectedObject = nullptr;
};
