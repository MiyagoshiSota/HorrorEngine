#include "SceneResourceManager.h"
#include "Scene/GameObject/GameObject.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHeap.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"

// 拡張子を置き換える処理
#include <filesystem>

#include "Core/App.h"
#include "Scene/GameObject/Model/Model.h"

struct SceneResourceManager::ModelData
{
	std::shared_ptr<Model> model;
	std::vector<SharedStruct::Mesh> origin_data;
};

void SceneResourceManager::initialize_gpu_resources_for(std::vector<SharedStruct::Mesh> origin_data,std::shared_ptr<Model> model)
{
	auto _model = ModelData{};

	_model.model = model;
	_model.origin_data = origin_data;

	// モデル分のMesh,Materialクラスを生成
	create_mesh_classes(_model);

	// 頂点バッファ、インデックスバッファの生成
	create_vertex_buffer(_model);
	create_index_buffer(_model);

	// マテリアルの読み込み
	read_material(_model);
}

void SceneResourceManager::create_vertex_buffer(ModelData model)
{
	// TODO:すでに生成済みだったら早期リーターン

	for (size_t i = 0; i < model.origin_data.size(); i++)
	{
		auto mesh = model.model->m_Meshes[i];
		auto size = sizeof(SharedStruct::Vertex) * model.origin_data[i].Vertices.size();
		auto data = model.origin_data[i].Vertices.data();

		if (!mesh->create_vertex_buffer(size, data))
		{
			printf("頂点バッファの生成に失敗\n");
			return;
		}

		// いらないのでは？
		//model.model->m_Meshes.push_back(mesh);
	}
}

void SceneResourceManager::create_index_buffer(ModelData model)
{
	// TODO:すでに生成済みだったら早期リーターン

	for (size_t i = 0; i < model.origin_data.size(); i++)
	{
		auto mesh = model.model->m_Meshes[i];
		auto size = sizeof(uint32_t) * model.origin_data[i].Indeices.size();
		auto data = model.origin_data[i].Indeices.data();

		if (!mesh->create_index_buffer(size, data))
		{
			printf("頂点バッファの生成に失敗\n");
			return;
		}
	}

}

/// <summary>
/// Textureを読み込んでヒープを確保
/// </summary>
/// <param name="obj"></param>
void SceneResourceManager::read_material(ModelData model)
{
	for (size_t i = 0; i < model.origin_data.size(); i++)
	{
		auto material = model.model->m_Materials[i];

		// 各テクスチャの確保
		// AlbedoMap
		material->set_texture("_MainTex", model.origin_data[i].hAlbedoMap);

		// Normal
		if (model.origin_data[i].HasNormalMap)
		{
			material->set_texture("_NormalMap",model.origin_data[i].hNormalMap);
		}

		// Metallic
		if (model.origin_data[i].HasMetallicMap)
		{
			material->set_texture("_MetallicRoughnessMap",model.origin_data[i].hMetallicMap);
		}

		// Emissive
		if (model.origin_data[i].HasEmissiveMap)
		{
			material->set_texture("_EmissiveMap",model.origin_data[i].hEmissiveMap);
		}

		// 色の設定
		material->set_color(model.origin_data[i].albedoFactor);
	}
}

void SceneResourceManager::create_mesh_classes(ModelData model)
{
	model.model->m_Meshes.clear();
	model.model->m_Materials.clear();

	// メッシュの数だけMeshクラスを生成
	for (size_t i = 0; i < model.origin_data.size(); i++)
	{
		auto mesh = std::make_shared<Mesh>();
		model.model->m_Meshes.push_back(mesh);
	}

	//	マテリアルの数だけMaterialクラスを生成
	for (size_t i = 0; i < model.origin_data.size(); i++)
	{
		auto material = std::make_shared<Material>();
		model.model->m_Materials.push_back(material);
	}
}
