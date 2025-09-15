#include "SceneBase.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Renderer/Graphics/RootSignature.h"
#include "Renderer/Graphics/DescriptorHeap.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include <d3dx12.h>

SceneBase* g_Scene;

using namespace DirectX;

const wchar_t* modelFile = L"../../assets/Alicia/Alicia/Alicia/FBX/Alicia_solid_Unity.FBX";
std::vector<SharedStruct::Mesh> meshes;


bool SceneBase::Init()
{
	ImportSettings importSetting = // これ自体は自作の読み込み設定構造体
	{
		modelFile,
		meshes,
		false,
		true
	};

	AssimpLoader loader;
	if (!loader.Load(importSetting))
	{
		return false;
	}

	// リソースの確保
	for (auto& obj : m_GameObjects)
	{
		m_SceneResourceManager->InitializeGpuResourcesFor(obj);
	}

	// カメラの初期化
	m_Camera = std::make_unique<SceneCamera>();
	m_Camera->SetPosition();
	m_Camera->LookAt();

	// レンダラーの初期化
	m_Renderer = std::make_unique<SceneRenderer>((L"../x64/Debug/SimpleVS.cso"), (L"../x64/Debug/SimplePS.cso"));

	printf("シーンの初期化に成功\n");
	return true;
}

void SceneBase::Update()
{

}

void SceneBase::Draw()
{
	auto currentIndex = g_Engine->CurrentBackBufferIndex();
	auto commandList = g_Engine->CommandList();

	// ゲームオブジェクトを描画
	for (auto& obj : m_GameObjects)
	{
		m_Renderer->DrawGameObject(commandList,obj);
	}
}


