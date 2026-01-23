#pragma once
#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Scene/SceneManager.h"
#include "Modules/PublicConst/ConstDayPref.h"
#include <string>
#include <cstring>
#include <filesystem>

class DrawDayWindow : public IDrawWindow
{
private:
    bool m_showSaveDialog = false;
    char m_dayNameBuffer[256] = "";

public:
    // 保存ダイアログを表示する（DrawMainMenuBarから呼び出される）
    void ShowSaveDialog()
    {
        m_showSaveDialog = true;
        m_dayNameBuffer[0] = '\0'; // バッファをクリア
    }

    void draw() override
    {
        // 名前入力ダイアログの表示
        if (m_showSaveDialog)
        {
            ImGui::OpenPopup(ConstDayPref::kSaveDialogTitle);
        }

        // モーダルダイアログ
        if (ImGui::BeginPopupModal(ConstDayPref::kSaveDialogTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text(ConstDayPref::kSaveDialogPrompt);
            ImGui::Spacing();
            
            ImGui::InputText("##DayName", m_dayNameBuffer, sizeof(m_dayNameBuffer));
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            bool canSave = strlen(m_dayNameBuffer) > 0;
            
            if (ImGui::Button("Save", ImVec2(120, 0)) && canSave)
            {
                // Game/Assets/Daysディレクトリのパスを作成
                std::filesystem::path daysDir = ConstDayPref::kDaysDirectoryPathGame;
                
                // ディレクトリが存在しない場合は作成
                if (!std::filesystem::exists(daysDir))
                {
                    std::filesystem::create_directories(daysDir);
                }
                
                // 新しいDayのファイルパスを作成
                std::filesystem::path dayFileName = std::string(m_dayNameBuffer) + ConstDayPref::kDayFileExtension;
                std::filesystem::path newDayPath = daysDir / dayFileName;
                
                // JSONファイルを保存
                if (g_Scene->SerializeGameObjects(newDayPath.string()))
                {
                    // 保存成功後、シーンマネージャーのパスを更新（シーンを再読み込みせずにパスのみ更新）
                    // シーンマネージャーでは相対パス形式を使用するため、Assets/Days/形式に変換
                    std::string scenePath = ConstDayPref::kDaysDirectoryPath;
                    scenePath += m_dayNameBuffer;
                    scenePath += ConstDayPref::kDayFileExtension;
                    g_SceneManager->UpdateCurrentScenePath(scenePath);
                    
                    m_showSaveDialog = false;
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    // 保存失敗時の処理（必要に応じてエラーメッセージを表示）
                }
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_showSaveDialog = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    };
};
