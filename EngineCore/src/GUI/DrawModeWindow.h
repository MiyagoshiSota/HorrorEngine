#pragma once
#include "IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Scene/SceneManager.h"

class DrawModeWindow : public IDrawWindow
{
public:
    void draw() override
    {
        ImGui::Begin("Draw Mode Window");
        
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

        // 現在のシーンモード表示
        ImGui::Text("CurrentMode:%s", g_scene_type == scene_type::editor_mode ? "EditorMode" : "PlayMode");

        ImGui::End();
    }

private:
    std::vector<std::shared_ptr<GameObject>> latest_objects;
};
