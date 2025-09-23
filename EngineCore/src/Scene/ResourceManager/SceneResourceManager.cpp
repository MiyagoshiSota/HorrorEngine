#include "SceneResourceManager.h"
#include "Scene/GameObject/GameObjectBase.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include "Renderer/Graphics/DescriptorHeap.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"

// 拡張子を置き換える処理
#include <filesystem>
namespace fs = std::filesystem;
std::wstring ReplaceExtension(const std::wstring& origin, const char* ext)
{
	fs::path p = origin.c_str();
	return p.replace_extension(ext).c_str();
}

void SceneResourceManager::InitializeGpuResourcesFor(std::shared_ptr<GameObjectBase> obj)
{
	CreateVertexBuffer(obj->m_Model);
	CreateIndexBuffer(obj->m_Model);
	ReadMaterial(obj->m_Model);
}

void SceneResourceManager::CreateVertexBuffer(std::shared_ptr<Model> model)
{
	model->m_Meshes->m_VertexBuffer.resize(model->m_InputMesh.size());
	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto size = sizeof(SharedStruct:: Vertex) * model->m_InputMesh[i].Vertices.size();
		auto stride = sizeof(SharedStruct::Vertex);
		auto vertices = model->m_InputMesh[i].Vertices.data();
		auto pVB = std::make_unique<VertexBuffer>(size, stride, vertices);
		if (!pVB->IsValid())
		{
			printf("頂点バッファの生成に失敗\n");
			return;
		}

		model->m_Meshes->m_VertexBuffer[i] = std::move(pVB);
	}
}

void SceneResourceManager::CreateIndexBuffer(std::shared_ptr<Model> model)
{
	model->m_Meshes->m_IndexBuffers.resize(model->m_InputMesh.size());
	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto size = sizeof(uint32_t) * model->m_InputMesh[i].Indeices.size();
		auto indices = model->m_InputMesh[i].Indeices.data();
		auto pIB = std::make_unique<IndexBuffer>(size, indices);
		if (!pIB->IsValid())
		{
			printf("インデックスバッファの生成に失敗\n");
			return;
		}

		model->m_Meshes->m_IndexBuffers[i] = std::move(pIB);
	}
}

/// <summary>
/// Textureを読み込んでヒープを確保
/// </summary>
/// <param name="obj"></param>
void SceneResourceManager::ReadMaterial(std::shared_ptr<Model> model)
{
	model->m_Material->m_DescriptorHeap = std::make_unique<DescriptorHeap>();
	model->m_Material->m_MaterialHandles.clear();

	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto texPath = ReplaceExtension(model->m_InputMesh[i].DiffuseMap, "tga"); // もともとはpsdになっていてちょっとめんどかったので、同梱されているtgaを読み込む
		auto mainTex = Texture2D::Get(texPath);
		auto handle = model->m_Material->m_DescriptorHeap->Register(mainTex);
		//model->m_Material->m_MaterialHandles.insert(model->m_Material->m_MaterialHandles.begin(), std::move(handle));
		model->m_Material->m_MaterialHandles.push_back(std::move(handle));
	}
}
