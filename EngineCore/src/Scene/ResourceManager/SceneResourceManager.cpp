#include "SceneResourceManager.h"
#include "Scene/GameObject/GameObject.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include "Renderer/Graphics/DescriptorHeap/SrvDescriptorHeap.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"

// 拡張子を置き換える処理
#include <filesystem>

#include "Modules/Other/engineString.h"
#include "Renderer/Engine.h"

void SceneResourceManager::initialize_gpu_resources_for(std::shared_ptr<GameObject> obj)
{
	auto model = obj->get_model();

	// モデル分のMesh,Materialクラスを生成
	create_mesh_classes(model);

	// 頂点バッファ、インデックスバッファの生成
	create_vertex_buffer(model);
	create_index_buffer(model);

	// マテリアルの読み込み
	read_material(model);
}

void SceneResourceManager::create_vertex_buffer(std::shared_ptr<Model> model)
{
	// TODO:すでに生成済みだったら早期リーターン

	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto mesh = model->m_Meshes[i];
		auto size = sizeof(SharedStruct::Vertex) * model->m_InputMesh[i].Vertices.size();
		auto data = model->m_InputMesh[i].Vertices.data();

		if (!mesh->create_vertex_buffer(size, data))
		{
			printf("頂点バッファの生成に失敗\n");
			return;
		}

		model->m_Meshes.push_back(mesh);
	}
}

void SceneResourceManager::create_index_buffer(std::shared_ptr<Model> model)
{
	// TODO:すでに生成済みだったら早期リーターン

	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto mesh = model->m_Meshes[i];
		auto size = sizeof(uint32_t) * model->m_InputMesh[i].Indeices.size();
		auto data = model->m_InputMesh[i].Indeices.data();

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
void SceneResourceManager::read_material(std::shared_ptr<Model> model)
{
	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto material = model->m_Materials[i];
		material->create_material(model->m_InputMesh[i].DiffuseMap);
		material->set_color(model->m_InputMesh[i].DiffuseColor);
		model->m_Materials.push_back(material);
	}
}

void SceneResourceManager::create_mesh_classes(std::shared_ptr<Model> meshes)
{
	meshes->m_Meshes.clear();
	meshes->m_Materials.clear();

	// メッシュの数だけMeshクラスを生成
	for (size_t i = 0; i < meshes->m_InputMesh.size(); i++)
	{
		auto mesh = std::make_shared<Mesh>();
		meshes->m_Meshes.push_back(mesh);
	}

	//	マテリアルの数だけMaterialクラスを生成
	for (size_t i = 0; i < meshes->m_InputMesh.size(); i++)
	{
		auto material = std::make_shared<Material>();
		meshes->m_Materials.push_back(material);
	}
}
