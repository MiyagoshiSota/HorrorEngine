#pragma once
#include <string>

#include "IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Scene/Default/Scene/DefaultScene.h"

class DrawPostProcessPresetWindow : public IDrawWindow
{
public:
    DrawPostProcessPresetWindow()
    {
        
    };
    
    void draw() override
    {
        // ImGui ウィンドウ作成
        ImGui::Begin("Example");

        // ドロップダウンメニュー（コンボボックス）を開始
        if (ImGui::BeginCombo("Preset", m_currentPresetName.c_str()))
        {
            auto scene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
            if (scene != nullptr)
            {
                auto ppManager = scene->get_post_process_manager();
                // マネージャーが保持している全てのプリセット名を取得してループ
                for (const auto& presetName : ppManager->get_preset_names())
                {
                    const bool isSelected = (m_currentPresetName == presetName);
                    if (ImGui::Selectable(presetName.c_str(), isSelected))
                    {
                        // 新しいプリセットが選択された
                        m_currentPresetName = presetName;

                        // 1秒かけて選択されたプリセットに移行する
                        ppManager->BlendToPreset(m_currentPresetName, 1.0f);
                    }
                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }   
            }
            ImGui::EndCombo();
        }
        
        ImGui::End();
    }

private:
    // 現在選択されているプリセットの名前を保持する変数
    std::string m_currentPresetName = "Normal";
};
