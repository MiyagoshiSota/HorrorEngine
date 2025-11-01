#include "ModelLoader.h"

#include "Core/App.h"
#include "Modules/PublicConst/const_path_pref.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/SceneManager.h"
#include "Scene/GameObject/DefaultMesh/DefaultMeshes.h"

bool ModelLoader::init()
{
    desirialize(const_path_pref::DefaultModelsPath);
    create_primitive_objects();
    return true;
}

std::shared_ptr<Model> ModelLoader::GetModelOrigin(const std::string& model_name)
{
    auto it = m_ModelCache.find(model_name);
    if (it != m_ModelCache.end())
    {
        return it->second;
    }
    return nullptr;
}

bool ModelLoader::desirialize(const std::string& models_file_path)
{
    std::ifstream file(models_file_path);
    if (!file.is_open())
    {
        printf("モデル設定ファイルのオープンに失敗:%s\n", models_file_path.c_str());
        return false;
    }
    
    nlohmann::json j;
    file >> j;
    
    for (const auto& item : j["models"])
    {
        // TODO:Keyがベタ書きされているのを修正
        std::string model_name = item["model_name"];
        std::string model_path = item["model_path"];

        // なんも入ってなかったらエラー
        if (model_name.empty() || model_path.empty())
        {
            printf("モデル設定ファイルのフォーマットが不正:%s\n", models_file_path.c_str());
            return false;
        }
        
        set_model(model_name, model_path);
    }
    return true;
}


std::shared_ptr<Model> ModelLoader::GetModel(const std::string& modelName)
{
    // TODO:全ModelがInputMeshを持っていて重複しているのは無駄づかいなので、InputMeshのみを持っているクラスと、InputMeshから確保したBufferを管理するクラスを分ける
    // リストからモデルを探してInputMeshのみコピーして返す
    auto it = m_ModelCache.find(modelName);
    if (it != m_ModelCache.end())
    {
        auto model = std::make_shared<Model>();
        model->m_InputMesh = it->second->m_InputMesh;
        return model;
    }
    return nullptr;
}

bool ModelLoader::set_model(const std::string& model_name, const std::string& model_path)
{
    // 既に登録されている場合はスルー
    if (m_ModelCache.find(model_name) != m_ModelCache.end())
    {
        printf("のmodel_nameは既に登録済みです。\n");
        return false;
    }
    
    auto model = std::make_shared<Model>();
    
    auto path2wst = engine_string::to_wstring(model_path);
    ImportSettings importSetting =
    {
        path2wst.c_str(),
        model->m_InputMesh,
        false,
        true
    };

    // モデルのロード
    AssimpLoader loader;
    if (!loader.Load(importSetting))
    {
        printf("モデルのロードに失敗:%s\n", model_path.c_str());;
        return false;
    }

    // キャッシュに登録
    m_ModelCache[model_name] = model;
    return true;
}

void ModelLoader::create_primitive_objects()
{
    // 平面
    auto quad_model = std::make_shared<Model>();
    quad_model->m_InputMesh.push_back(DefaultMeshes::create_quad());
    SceneResourceManager::GetInstance().create_mesh_classes(quad_model);
    SceneResourceManager::GetInstance().create_index_buffer(quad_model);
    SceneResourceManager::GetInstance().create_vertex_buffer(quad_model);
    m_ModelCache["primitive/quad"] = quad_model;

    // 立方体
    auto cube_model = std::make_shared<Model>();
    cube_model->m_InputMesh.push_back(DefaultMeshes::create_cube());
    SceneResourceManager::GetInstance().create_mesh_classes(cube_model);
    SceneResourceManager::GetInstance().create_index_buffer(cube_model);
    SceneResourceManager::GetInstance().create_vertex_buffer(cube_model);
    m_ModelCache["primitive/cube"] = cube_model;
}