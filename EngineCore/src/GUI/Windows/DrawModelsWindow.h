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
        const auto& models = g_ModelLoader->GetAllModelsData();
        for (const auto& [modelName, modelPtr] : models)
        {
            // 各モデルを選択したら、そのモデルを使って新しいゲームオブジェクトを作成
            if (ImGui::Button(modelName.c_str()))
            {
                // 新しいゲームオブジェクトを作成
                auto newGameObject = std::make_shared<GameObject>();
                newGameObject->m_name = SetNameFind(modelName);
                const auto sharedPtr = g_ModelLoader->GetModel(modelName);

                //　デフォルトのコンポーネントを設定
                // MeshRenderer
                auto rendererComponent = ComponentFactory::create("MeshRenderer");
                rendererComponent->Initialize(newGameObject);
                rendererComponent->Deserialize(nlohmann::json{{"model_name", modelName}});
                newGameObject->components.push_back(std::move(rendererComponent));
                
                // ゲームオブジェクトのInitを実行
                newGameObject->Init();

                // シーンに追加
                g_Scene->AddGameObject(newGameObject);
            }
            
        }
        
        ImGui::End();
    }

private:
    // 同じ名前のモデルが存在する場合、名前の末尾に連番を付与して新しい名前を生成する
    std::string SetNameFind(std::string modelName)
    {
        std::string baseName = modelName;
        std::string finalName = baseName;

        // 同名チェック
        int counter = 1;
        bool nameExists = true;

        while (nameExists)
        {
            nameExists = false;
            for (const auto& obj : g_Scene->GetGameObjects())
            {
                if (obj->GetName() == finalName)
                {
                    nameExists = true;
                    finalName = baseName + std::to_string(counter++);
                    break;
                }
            }
        }
        return finalName;
    }
};
