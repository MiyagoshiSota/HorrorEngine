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
            if (g_scene_type == scene_type::editor_mode)
            {
                change_scene_type(scene_type::play_mode);

                // g_Scene->SaveStateJson(); // 状態を保存
            }
            else
            {
                change_scene_type(scene_type::editor_mode);
            }

            // シーン切り替え時の初期化処理
            // TODO:一部冗長な初期化処理が入ってる気がするので整理する
            g_Scene->RebuidPhysicsWorld(); // 物理ワールドを再構築
            g_Scene->CreatePrimitiveObjects(); // プリミティブオブジェクトの再生成
            g_Scene->InitializeGameObject(g_SceneManager->GetNextScenePath()); // ゲームオブジェクトの初期化
            g_Scene->get_audio_manager()->init(); // オーディオマネージャのリセット
            g_Scene->get_time_manager()->reset(); // タイムマネージャのリセット

            printf("ゲームオブジェクトの初期化");
        }

        // 現在のシーンモード表示
        ImGui::Text("CurrentMode:%s", g_scene_type == scene_type::editor_mode ? "EditorMode" : "PlayMode");

        ImGui::End();
    }
};
