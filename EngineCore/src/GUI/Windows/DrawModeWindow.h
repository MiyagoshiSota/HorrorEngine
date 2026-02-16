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
            
            if (g_sceneType == SceneType::EditorMode)
            {
                // 先に一時保存してから Play に切り替える（保存失敗時は Play にしない）
                if (!g_Scene->SerializeGameObjects(ConstPathPref::kDefaultTempGameObjectPath))
                    return;
                ChangeSceneType(SceneType::PlayMode);
            }
            else
            {
                // プレイモードからエディターモードに戻る際の処理
                g_Scene->RebuidPhysicsWorld(); // 物理ワールドを再構築
                g_Scene->InitializeGameObject(ConstPathPref::kDefaultTempGameObjectPath);
                
                ChangeSceneType(SceneType::EditorMode);
            }

            // シーン切り替え時の初期化処理
            
            // ゲームオブジェクトのInit処理
            for (auto& obj : g_Scene->GetGameObjects())
            {
                obj->Init();
            }
            g_Scene->GetAudioManager()->Init(); // オーディオマネージャのリセット
            g_Scene->GetTimeManager()->Reset(); // タイムマネージャのリセット

            printf("ゲームオブジェクトの初期化\n");
        }

        ImGui::SameLine();
        
        // 現在のシーンモード表示
        ImGui::Text("CurrentMode:%s", g_sceneType == SceneType::EditorMode ? "EditorMode" : "PlayMode");
    }

private:
    std::vector<std::shared_ptr<GameObject>> latest_objects;
};
