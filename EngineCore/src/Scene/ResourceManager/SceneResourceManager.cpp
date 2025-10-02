#include "SceneResourceManager.h"
#include "Scene/GameObject/IGameObjectBase.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include "Renderer/Graphics/DescriptorHeap/SrvDescriptorHeap.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"

// 拡張子を置き換える処理
#include <filesystem>

#include "Renderer/Engine.h"
namespace fs = std::filesystem;
std::wstring ReplaceExtension(const std::wstring& origin, const char* ext)
{
	fs::path p = origin.c_str();
	return p.replace_extension(ext).c_str();
}

void SceneResourceManager::InitializeGpuResourcesFor(std::shared_ptr<IGameObjectBase> obj)
{
	CreateVertexBuffer(obj->GetModel());
	CreateIndexBuffer(obj->GetModel());
	ReadMaterial(obj->GetModel());
}

void SceneResourceManager::CreateVertexBuffer(std::shared_ptr<Model> model)
{
	// すでに生成済みだったら早期リーターン
	if (!model->m_Meshes->m_VertexBuffer.empty() && model->m_Meshes->m_VertexBuffer[0] != nullptr)
	{
		return;
	}

	model->m_Meshes->m_VertexBuffer.resize(model->m_InputMesh.size());
	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto size = sizeof(SharedStruct:: Vertex) * model->m_InputMesh[i].Vertices.size();
		auto stride = sizeof(SharedStruct::Vertex);
		auto vertices = model->m_InputMesh[i].Vertices.data();
		auto pVB = std::make_shared<VertexBuffer>(size, stride, vertices);
		if (!pVB->IsValid())
		{
			printf("頂点バッファの生成に失敗\n");
			return;
		}

		model->m_Meshes->m_VertexBuffer[i] = pVB;
	}
}

void SceneResourceManager::CreateIndexBuffer(std::shared_ptr<Model> model)
{
	// すでに生成済みだったら早期リーターン
	if (!model->m_Meshes->m_IndexBuffers.empty() && model->m_Meshes->m_IndexBuffers[0] != nullptr)
	{
		return;
	}

	model->m_Meshes->m_IndexBuffers.resize(model->m_InputMesh.size());
	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		auto size = sizeof(uint32_t) * model->m_InputMesh[i].Indeices.size();
		auto indices = model->m_InputMesh[i].Indeices.data();
		auto pIB = std::make_shared<IndexBuffer>(size, indices);
		if (!pIB->IsValid())
		{
			printf("インデックスバッファの生成に失敗\n");
			return;
		}

		model->m_Meshes->m_IndexBuffers[i] = pIB;
	}
}

/// <summary>
/// Textureを読み込んでヒープを確保
/// </summary>
/// <param name="obj"></param>
void SceneResourceManager::ReadMaterial(std::shared_ptr<Model> model)
{
	model->m_Material->m_DescriptorHeap = g_Engine->GetSrvHeap();
	model->m_Material->m_MaterialHandles.clear();

	for (size_t i = 0; i < model->m_InputMesh.size(); i++)
	{
		// ハンドル確保し取得
		auto m_SrvHandle = g_Engine->GetSrvHeap()->Allocate();
		if (m_SrvHandle == nullptr) {
			// ハンドル確保失敗
			return;
		}

		// MainTexture分のSRV生成
		auto texPath = ReplaceExtension(model->m_InputMesh[i].DiffuseMap, "tga"); // もともとはpsdになっていてちょっとめんどかったので、同梱されているtgaを読み込む
		auto mainTex = Texture2D::Get(texPath);
		auto desc = mainTex->ViewDesc();
		g_Engine->Device()->CreateShaderResourceView(mainTex->Resource().Get(), &desc, m_SrvHandle->cpuHandle); // シェーダーリソースビュー作成
		model->m_Material->m_MaterialHandles.push_back(m_SrvHandle);
	}
}
