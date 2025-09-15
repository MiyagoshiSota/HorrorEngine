#include "SceneResourceManager.h"
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/GameObjectBase.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"

void SceneResourceManager::InitializeGpuResourcesFor(GameObjectBase& obj)
{
	CreateVertexBuffer(obj.m_Model->m_Meshes);
	CreateIndexBuffer(obj);
	ReadMaterial(obj);
}

void SceneResourceManager::CreateVertexBuffer(Meshes obj)
{
	
	vertexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(Vertex) * meshes[i].Vertices.size();
		auto stride = sizeof(Vertex);
		auto vertices = meshes[i].Vertices.data();
		auto pVB = new VertexBuffer(size, stride, vertices);
		if (!pVB->IsValid())
		{
			printf("頂点バッファの生成に失敗\n");
			return;
		}

		vertexBuffers.push_back(pVB);
	}
}

void SceneResourceManager::CreateIndexBuffer(GameObjectBase& obj)
{
	indexBuffers.reserve(meshes.size());
	for (size_t i = 0; i < meshes.size(); i++)
	{
		auto size = sizeof(uint32_t) * meshes[i].Indeices.size();
		auto indices = meshes[i].Indeices.data();
		auto pIB = new IndexBuffer(size, indices);
		if (!pIB->IsValid())
		{
			printf("インデックスバッファの生成に失敗\n");
			return false;
		}

		indexBuffers.push_back(pIB);
	}
}

/// <summary>
/// Textureを読み込んでヒープを確保
/// </summary>
/// <param name="obj"></param>
void SceneResourceManager::ReadMaterial(GameObjectBase& obj)
{
}
