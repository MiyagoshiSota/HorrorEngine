#include "ModelLoader.h"

#include "Core/App.h"
#include "Modules/Other/engineString.h"
#include "Modules/PublicConst/const_path_pref.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/GameObject/DefaultMesh/DefaultMeshes.h"
#include "Scene/GameObject/Model/Model.h"

bool ModelLoader::init()
{
	// モデル設定ファイルからモデルを読み込み
    desirialize(const_path_pref::DefaultModelsPath);

	// プリミティブオブジェクトの作成
    create_primitive_objects();
    return true;
}

std::vector<SharedStruct::Mesh> ModelLoader::GetModelOriginData(const std::string& model_name)
{
    auto it = m_ModelDataCache.find(model_name);
    if (it != m_ModelDataCache.end())
    {
        return it->second;
    }

	printf("モデルが見つかりません:%s\n", model_name.c_str());
	return {};
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
    auto it = m_ModelCache.find(modelName);
    if (it != m_ModelCache.end())
    {
		return it->second;
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
    
    std::vector<SharedStruct::Mesh> input_data = {};
    
    auto path2wst = engine_string::to_wstring(model_path);
    ImportSettings importSetting =
    {
        path2wst.c_str(),
        input_data,
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

    // OriginDataのキャッシュに登録
    m_ModelDataCache[model_name] = input_data;

	// Modelオブジェクトの作成とキャッシュ登録
	auto model = std::make_shared<Model>(model_name);
	SceneResourceManager::GetInstance().initialize_gpu_resources_for(input_data, model);
	m_ModelCache[model_name] = model;

    return true;
}

void ModelLoader::create_primitive_objects()
{
    // ~~平面~~
	const auto quad_name = "primitive/quad";
    auto quad_model = std::make_shared<Model>(quad_name);
    std::vector<SharedStruct::Mesh> quad_origin_data;

	// 平面メッシュの作成
	quad_origin_data.clear();
	quad_origin_data.push_back(DefaultMeshes::create_quad());

	// モデルデータの作成
    SceneResourceManager::GetInstance().initialize_gpu_resources_for(quad_origin_data,quad_model);

	// キャッシュに登録
    m_ModelDataCache[quad_name] = quad_origin_data;
    m_ModelCache[quad_name] = quad_model;

    // ~~立方体~~
	const auto cube_name = "primitive/cube";
    auto cube_model = std::make_shared<Model>(cube_name);
    std::vector<SharedStruct::Mesh> cube_origin_data;

	// 立方体メッシュの作成
	cube_origin_data.clear();
    cube_origin_data.push_back(DefaultMeshes::create_cube());

	// モデルデータの作成
    SceneResourceManager::GetInstance().initialize_gpu_resources_for(cube_origin_data, cube_model);
	
	// キャッシュに登録
    m_ModelDataCache[cube_name] = cube_origin_data;
    m_ModelCache[cube_name] = cube_model;
}