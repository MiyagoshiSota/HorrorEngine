#pragma once
#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Scene/SceneManager.h"

class DrawModeWindow : public IDrawWindow
{
public:
    // ウィンドウとして描画（従来の方法、現在は使用しない）
    void draw() override
    {
        // 固定表示: 画面上部中央に固定、移動・リサイズ・ドッキング不可
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 windowPos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f - 130.0f, viewport->WorkPos.y + 19.0f); // メニューバーの下、中央
        ImVec2 windowSize = ImVec2(260.0f, 100.0f);
        
        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
        
        // 固定ウィンドウのフラグ: 移動不可、リサイズ不可、ドッキング不可、折りたたみ不可
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | 
                                  ImGuiWindowFlags_NoResize | 
                                  ImGuiWindowFlags_NoDocking |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoSavedSettings; // 位置・サイズを.iniに保存しない

        if (!ImGui::Begin("Draw Mode Window", &m_isVisible, flags))
        {
            ImGui::End();
            return;
        }
        
        draw_content();
        ImGui::End();
    }
    
    // メニューバー内など、ウィンドウの枠なしで内容だけを描画
    void draw_content()
    {
        // シーン切り替えボタン
        if (ImGui::Button("ChangeMode")) {
            g_Engine->WaitForGPU(); // GPUの処理が終わるまで待つ
            
            if (g_scene_type == scene_type::editor_mode)
            {
                change_scene_type(scene_type::play_mode);

                g_Scene->serialize_game_objects(const_path_pref::DefaultTempGameObjectPath); // 一時保存
            }
            else
            {
                // プレイモードからエディターモードに戻る際の処理
                g_Scene->RebuidPhysicsWorld(); // 物理ワールドを再構築
                g_Scene->InitializeGameObject(const_path_pref::DefaultTempGameObjectPath);
                
                change_scene_type(scene_type::editor_mode);
            }

            // シーン切り替え時の初期化処理
            
            // ゲームオブジェクトのInit処理
            for (auto& obj : g_Scene->get_game_objects())
            {
                obj->init();
            }
            g_Scene->get_audio_manager()->init(); // オーディオマネージャのリセット
            g_Scene->get_time_manager()->reset(); // タイムマネージャのリセット

            printf("ゲームオブジェクトの初期化");
        }

        ImGui::SameLine();
        
        // 現在のシーンモード表示
        ImGui::Text("CurrentMode:%s", g_scene_type == scene_type::editor_mode ? "EditorMode" : "PlayMode");
    }

private:
    std::vector<std::shared_ptr<GameObject>> latest_objects;
};
