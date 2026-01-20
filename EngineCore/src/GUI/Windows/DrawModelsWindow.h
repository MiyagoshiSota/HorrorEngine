#pragma once
#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Core/Components/TriggerComponent.h"
#include "Scene/GameObject/Component/ComponentFactory.h"
#include "Scene/GameObject/Component/MeshRenderer.h"

class DrawModelsWindow : public IDrawWindow
{
public:
    void draw() override
    {
        // 最小サイズ制限を設定（モデル一覧が表示可能なサイズ）
        ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));

        if (!ImGui::Begin("Models Window", &m_isVisible))
        {
            ImGui::End();
            return;
        }

        // 登録されているモデルの一覧を表示
        const auto& models = g_ModelLoader->get_all_models_data();
        for (const auto& [model_name, model_ptr] : models)
        {
            // 各モデルを選択したら、そのモデルを使って新しいゲームオブジェクトを作成
            if (ImGui::Button(model_name.c_str()))
            {
                // 新しいゲームオブジェクトを作成
                auto new_game_object = std::make_shared<GameObject>();
                new_game_object->name = set_name_find(model_name);
                const auto shared_ptr = g_ModelLoader->GetModel(model_name);

                //　デフォルトのコンポーネントを設定
                // MeshRenderer
                auto renderer_component = ComponentFactory::create("MeshRenderer");
                renderer_component->initialize(new_game_object);
                renderer_component->deserialize(nlohmann::json{{"model_name", model_name}});
                new_game_object->components.push_back(std::move(renderer_component));
                
                // ゲームオブジェクトのInitを実行
                new_game_object->init();

                // シーンに追加
                g_Scene->add_game_object(new_game_object);
            }
            
        }
        
        ImGui::End();
    }

private:
    // 同じ名前のモデルが存在する場合、名前の末尾に連番を付与して新しい名前を生成する
    std::string set_name_find(std::string model_name )
    {
        std::string base_name = model_name;
        std::string final_name = base_name;

        // 同名チェック
        int counter = 1;
        bool name_exists = true;

        while (name_exists)
        {
            name_exists = false;
            for (const auto& obj : g_Scene->get_game_objects())
            {
                if (obj->get_name() == final_name)
                {
                    name_exists = true;
                    final_name = base_name + std::to_string(counter++);
                    break;
                }
            }
        }
        return final_name;
    }
};
