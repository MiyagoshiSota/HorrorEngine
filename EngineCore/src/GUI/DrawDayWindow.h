#pragma once
#include "IDrawWindow.h"
#include "imgui.h"
#include "Scene/SceneManager.h"

class DrawDayWindow : public IDrawWindow
{
public:
    void draw() override
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Day")) {
                    // 新しいDayを作成する処理を呼び出す
                }
                if (ImGui::BeginMenu("Open Day"))
                {
                    // ここで利用可能なDayのリストを表示する
                    if (ImGui::MenuItem("Day1_Apartment")) {
                        g_SceneManager->LoadScene("assets/Days/Day1.json");
                        g_SceneManager->ProcessSceneRequest();
                    }
                    
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Save Day")) {
                    // 現在のDayを保存する処理を呼び出す
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                // ... (Undo, Redoなど) ...
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    };
};
