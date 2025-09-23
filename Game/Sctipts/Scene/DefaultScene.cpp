#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Renderer/Graphics/RootSignature.h"
#include "Renderer/Graphics/DescriptorHeap.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include "../GameObject/DefaultGameObject.h"
#include <d3dx12.h>

using namespace DirectX;

const wchar_t* modelFile[2] = { L"assets/Alicia/Alicia/FBX/Alicia_solid_Unity.FBX" , L"assets/walking/rp_nathan_animated_003_walking.fbx" };
const int gameobjectCount = 4;
std::vector<std::shared_ptr<Model>> models;


bool DefaultScene::Init()
{
	// モデルのロード
	for (size_t i = 0; i < std::size(modelFile) - 1; i++)
	{
		models.push_back(std::make_shared<Model>());
		ImportSettings importSetting =
		{
			modelFile[i],
			models[i]->m_InputMesh,
			false,
			true
		};

		AssimpLoader loader;
		if (!loader.Load(importSetting))
		{
			return false;
		}
	}

	// ゲームオブジェクトの生成
	for (size_t i = 0; i < gameobjectCount; i++)
	{
		std::shared_ptr<IGameObjectBase> gameObj = std::make_shared<DefaultGameObject>(models[0]);

		// オブジェクト専用の定数バッファを作成
		gameObj->CreateConstantBuffer((sizeof(SharedStruct::Transform)));
		m_GameObjects.push_back(gameObj);
	}

	// それぞれのオブジェクトに初期位置を設定
	for (auto& obj : m_GameObjects)
	{
		obj->SetPosition(0, 0, 0);
	}

	// リソースの確保
	for (auto& obj : m_GameObjects)
	{
		m_SceneResourceManager->InitializeGpuResourcesFor(obj);
	}

	// カメラの初期化
	m_Camera = std::make_unique<SceneCamera>();
	m_Camera->Init();

	// レンダラーの初期化
	m_Renderer = std::make_unique<SceneRenderer>((L"../x64/Debug/SimpleVS.cso"), (L"../x64/Debug/SimplePS.cso"));

	printf("シーンの初期化に成功\n");

	// ゲームオブジェクトのInitを実行
	for (auto& obj : m_GameObjects)
	{
		obj->Init();
	}
	
	printf("ゲームオブジェクトの初期化を設定");

	return true;
}

void DefaultScene::Update()
{
	ISceneBase::Update();
}

void DefaultScene::Draw()
{
	auto currentIndex = g_Engine->CurrentBackBufferIndex();
	auto commandList = g_Engine->CommandList();

	// パイプラインとルートシグネチャは全オブジェクトで共通なので最初にセット
	commandList->SetGraphicsRootSignature(m_Renderer->GetRootSignature());
	commandList->SetPipelineState(m_Renderer->GetPipelineState());

	// ゲームオブジェクトをループで描画
	for (auto& obj : m_GameObjects)
	{
		// 各オブジェクトが専用の定数バッファを持っている前提
		auto pTransform = obj->GetConstantBuffer()->GetPtr<SharedStruct::Transform>();
		pTransform->World = obj->GetTransform();
		pTransform->View = DirectX::XMMatrixLookAtRH(m_Camera->GetEyePos(), m_Camera->GetTargetPos(), m_Camera->GetUpward());
		pTransform->Proj = DirectX::XMMatrixPerspectiveFovRH(m_Camera->GetFOV(), m_Camera->GetAspect(), 0.3f, 1000.0f);

		// 更新した定数バッファを GPU にセット
		commandList->SetGraphicsRootConstantBufferView(0, obj->GetConstantBuffer()->GetAddress());

		// オブジェクトを描画
		m_Renderer->DrawGameObject(commandList, obj);
	}
}