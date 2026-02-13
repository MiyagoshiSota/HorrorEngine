#pragma once
#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Scene/SceneManager.h"
#include "Modules/PublicConst/ConstDayPref.h"
#include "Renderer/Engine.h"
#include "GUI/Windows/DrawGameObjectWindow.h"
#include "GUI/Windows/DrawPostProcessPresetWindow.h"
#include "GUI/Windows/DrawAAWindow.h"
#include "GUI/Windows/DrawModelsWindow.h"
#include "GUI/Windows/DrawTaskManagerWindow.h"
#include "GUI/Windows/DrawWorkManagerWindow.h"
#include "GUI/Windows/DrawPlayerWindow.h"
#include "GUI/Windows/DrawDayWindow.h"
#include "GUI/Windows/DrawModeWindow.h"
#include "GUI/Windows/DrawBuildWindow.h"
#include "GUI/Managers/LayoutPresetManager.h"
#include <string>
#include <filesystem>
#include <memory>

class DrawMainMenuBar : public IDrawWindow
{
public:
    void draw() override
    {
        // メニューバーの高さを調整（モードウィンドウ用に太くする）
        // BeginMainMenuBar()の前にスタイルを変更する必要がある
        ImGuiStyle& style = ImGui::GetStyle();
        float originalFramePaddingY = style.FramePadding.y;
        float originalItemSpacingY = style.ItemSpacing.y;
        
        // メニューバーを太くする（FramePaddingを大きくする）
        style.FramePadding.y = 12.0f; // 上下のパディングを大きく増やす
        
        if (ImGui::BeginMainMenuBar())
        {
            draw_file_menu();
            draw_edit_menu();
            draw_build_menu();
            draw_window_menu();
            draw_mode_window_in_center();
            
            ImGui::EndMainMenuBar();
        }
        
        // スタイルを元に戻す（EndMainMenuBar()の後）
        style.FramePadding.y = originalFramePaddingY;
        style.ItemSpacing.y = originalItemSpacingY;
    }

private:
    // Fileメニューの描画
    void draw_file_menu()
    {
        if (ImGui::BeginMenu("File"))
            {
                // 新しいシーンを作成
                if (ImGui::MenuItem("New Day")) {
                    g_SceneManager->LoadScene(ConstDayPref::kTmpDayPath);
                    g_SceneManager->ProcessSceneRequest();
                }

                // シーンを開く
                if (ImGui::BeginMenu("Open Day"))
                {
                    // シーンのリストを動的に生成する                    
                    std::filesystem::path daysDir = ConstDayPref::kDaysDirectoryPathGame; 
                    for (const auto& entry : std::filesystem::directory_iterator(daysDir))
                    {
                        // ファイルの拡張子が.jsonの場合
                        if (entry.is_regular_file() && entry.path().extension() == ConstDayPref::kDayFileExtension)
                        {
                            // ファイル名を取得
                            std::string dayName = entry.path().stem().string();
                            // メニューに追加
                            if (ImGui::MenuItem(dayName.c_str())) {
                                // シーンを読み込み
                                g_SceneManager->LoadScene(entry.path().string());
                                g_SceneManager->ProcessSceneRequest();
                            }
                        }
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Save Day")) {
                    auto now_scene = g_SceneManager->GetNextScenePath();
                    
                    // TmpDayの場合は名前入力ダイアログを表示
                    if (now_scene == ConstDayPref::kTmpDayPath || 
                        now_scene.find(ConstDayPref::kTmpDayFileName) != std::string::npos) {
                        // DrawDayWindowに保存ダイアログを表示させる
                        if (g_Engine)
                        {
                            auto& windows = g_Engine->GetDrawWindows();
                            for (auto& window : windows)
                            {
                                if (auto dayWindow = std::dynamic_pointer_cast<DrawDayWindow>(window))
                                {
                                    dayWindow->ShowSaveDialog();
                                    break;
                                }
                            }
                        }
                    } else {
                        // 通常の保存処理
                        g_Scene->SerializeGameObjects(now_scene);
                    }
                }
                ImGui::EndMenu();
            }
    }
    
    // Editメニューの描画
    void draw_edit_menu()
    {
        if (ImGui::BeginMenu("Edit"))
        {
            // ... (Undo, Redoなど) ...
            ImGui::EndMenu();
        }
    }
    
    // Buildメニューの描画
    void draw_build_menu()
    {
        if (ImGui::BeginMenu("Build"))
        {
            if (ImGui::MenuItem("Open Build Window"))
            {
                if (g_Engine)
                {
                    auto& windows = g_Engine->GetDrawWindows();
                    for (auto& window : windows)
                    {
                        if (auto buildWindow = std::dynamic_pointer_cast<DrawBuildWindow>(window))
                        {
                            buildWindow->SetVisible(true);
                            break;
                        }
                    }
                }
            }

            ImGui::EndMenu();
        }
    }
    
    // Windowメニューの描画
    void draw_window_menu()
    {
            if (ImGui::BeginMenu("Window"))
            {
                // レイアウトプリセット
                draw_layout_preset_menu();
                
                ImGui::Separator();
                
                // 各ウィンドウの表示/非表示を切り替え
                draw_window_visibility_menu();
                
                ImGui::EndMenu();
            }
    }
    
    // レイアウトプリセットメニューの描画
    void draw_layout_preset_menu()
    {
        if (ImGui::BeginMenu("Layout"))
        {
            auto& presetManager = LayoutPresetManager::GetInstance();
            LayoutPresetType currentPreset = presetManager.GetCurrentPreset();
            
            if (ImGui::MenuItem("Make Mode", nullptr, currentPreset == LayoutPresetType::MakeMode))
            {
                presetManager.ApplyPreset(LayoutPresetType::MakeMode);
                // 次フレームでレイアウトを読み込むように予約
                if (g_Engine)
                {
                    g_Engine->SchedulePresetLoad(LayoutPresetType::MakeMode);
                }
            }
            
            if (ImGui::MenuItem("Debug Mode", nullptr, currentPreset == LayoutPresetType::DebugMode))
            {
                presetManager.ApplyPreset(LayoutPresetType::DebugMode);
                // 次フレームでレイアウトを読み込むように予約
                if (g_Engine)
                {
                    g_Engine->SchedulePresetLoad(LayoutPresetType::DebugMode);
                }
            }
            
            ImGui::EndMenu();
        }
    }
    
    // ウィンドウの表示/非表示切り替えメニューの描画
    void draw_window_visibility_menu()
    {
        if (!g_Engine)
            return;
            
        auto& windows = g_Engine->GetDrawWindows();
        
        // 各ウィンドウを型で検索して表示/非表示を切り替え
        std::shared_ptr<DrawGameObjectWindow> gameObjectWindow;
        std::shared_ptr<DrawPostProcessPresetWindow> postProcessWindow;
        std::shared_ptr<DrawAAWindow> aaWindow;
        std::shared_ptr<DrawModelsWindow> modelsWindow;
        std::shared_ptr<DrawTaskManagerWindow> taskManagerWindow;
        std::shared_ptr<DrawWorkManagerWindow> workManagerWindow;
        std::shared_ptr<DrawPlayerWindow> playerWindow;
        
        // ウィンドウリストから各型のウィンドウを検索
        for (auto& window : windows)
        {
            if (!gameObjectWindow) gameObjectWindow = std::dynamic_pointer_cast<DrawGameObjectWindow>(window);
            if (!postProcessWindow) postProcessWindow = std::dynamic_pointer_cast<DrawPostProcessPresetWindow>(window);
            if (!aaWindow) aaWindow = std::dynamic_pointer_cast<DrawAAWindow>(window);
            if (!modelsWindow) modelsWindow = std::dynamic_pointer_cast<DrawModelsWindow>(window);
            if (!taskManagerWindow) taskManagerWindow = std::dynamic_pointer_cast<DrawTaskManagerWindow>(window);
            if (!workManagerWindow) workManagerWindow = std::dynamic_pointer_cast<DrawWorkManagerWindow>(window);
            if (!playerWindow) playerWindow = std::dynamic_pointer_cast<DrawPlayerWindow>(window);
        }
        
        // Game Object Window
        if (gameObjectWindow)
        {
            bool visible = gameObjectWindow->IsVisible();
            if (ImGui::MenuItem("Game Object Window", nullptr, &visible))
            {
                gameObjectWindow->SetVisible(visible);
            }
        }
        
        // Mode Window は固定UIなのでメニューから除外
        
        // Post Process Preset Window
        if (postProcessWindow)
        {
            bool visible = postProcessWindow->IsVisible();
            if (ImGui::MenuItem("Post Process Preset Window", nullptr, &visible))
            {
                postProcessWindow->SetVisible(visible);
            }
        }

        // Rendering Settings Window
        if (aaWindow)
        {
            bool visible = aaWindow->IsVisible();
            if (ImGui::MenuItem("Rendering Settings", nullptr, &visible))
            {
                aaWindow->SetVisible(visible);
            }
        }
        
        // Models Window
        if (modelsWindow)
        {
            bool visible = modelsWindow->IsVisible();
            if (ImGui::MenuItem("Models Window", nullptr, &visible))
            {
                modelsWindow->SetVisible(visible);
            }
        }
        
        // Task Manager Window
        if (taskManagerWindow)
        {
            bool visible = taskManagerWindow->IsVisible();
            if (ImGui::MenuItem("Task Manager Window", nullptr, &visible))
            {
                taskManagerWindow->SetVisible(visible);
            }
        }
        
        // Work Manager Window
        if (workManagerWindow)
        {
            bool visible = workManagerWindow->IsVisible();
            if (ImGui::MenuItem("Work Manager Window", nullptr, &visible))
            {
                workManagerWindow->SetVisible(visible);
            }
        }

        // Player Window
        if (playerWindow)
        {
            bool visible = playerWindow->IsVisible();
            if (ImGui::MenuItem("Player Window", nullptr, &visible))
            {
                playerWindow->SetVisible(visible);
            }
        }
    }
    
    // メニューバーの中央にモードウィンドウを配置
    void draw_mode_window_in_center()
    {
        if (!g_Engine)
            return;
            
        std::shared_ptr<IDrawWindow> modeWindow = g_Engine->GetModeWindow();
        if (!modeWindow)
            return;
        
        // 中央に配置するためのスペーシング
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarWidth = viewport->WorkSize.x;
        float leftMenuWidth = ImGui::GetCursorPosX(); // 左側のメニュー項目の幅
        
        // モードウィンドウの推定幅を計算（ボタン + テキスト）
        float modeWindowWidth = 200.0f; // おおよその幅
        float centerX = menuBarWidth * 0.5f;
        float targetX = centerX - modeWindowWidth * 0.5f;
        
        // 左側のメニューより右側に配置し、中央に近づける
        if (targetX > leftMenuWidth + 20.0f)
        {
            ImGui::SetCursorPosX(targetX);
        }
        else
        {
            ImGui::SameLine(0, 20.0f); // スペースが足りない場合は少し右に
        }
        
        // モードウィンドウの内容を描画
        if (auto modeWindowTyped = std::dynamic_pointer_cast<DrawModeWindow>(modeWindow))
        {
            modeWindowTyped->draw_content();
        }
    }
};
