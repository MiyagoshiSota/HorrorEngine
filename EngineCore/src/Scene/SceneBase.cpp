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

const wchar_t* modelFile[2] = { L"assets/Alicia/Alicia/FBX/Alicia_solid_Unity.FBX" , L"assets/walking/rp_nathan_animated_003_walking.fbx" };
const int modelCount = 1;
const int gameobjectCount = 4;
std::vector<std::shared_ptr<Model>> models;


bool SceneBase::Init()
{
	for (size_t i = 0; i < modelCount; i++)
	{
		models.push_back(std::make_shared<Model>());
		ImportSettings importSetting = 
		{
			modelFile[0],
			models[i]->m_InputMesh,
			// TODO:UV座標の反転を各モデルで考慮しなきゃね
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
		auto gameObj = std::make_unique<GameObjectBase>(models[0]);

		// オブジェクト専用の定数バッファを作成
		gameObj->constantBuffer = new ConstantBuffer(sizeof(SharedStruct::Transform));
		m_GameObjects.push_back(std::move(gameObj));
	}


	// それぞれのオブジェクトに位置を設定
	float pos = 0;
	for (auto& obj : m_GameObjects)
	{
		obj->SetPosition(pos, pos, 0);
		pos += 20;
	}

	// リソースの確保
	for (auto& obj : m_GameObjects)
	{
		m_SceneResourceManager->InitializeGpuResourcesFor(obj);
	}

	// カメラの初期化
	m_Camera = std::make_unique<SceneCamera>();
	m_Camera->SetInformation();
	SetConstantBuffer();

	// レンダラーの初期化
	m_Renderer = std::make_unique<SceneRenderer>((L"../x64/Debug/SimpleVS.cso"), (L"../x64/Debug/SimplePS.cso"));

	printf("シーンの初期化に成功\n");
	return true;
}

void SceneBase::Update()
{
	// 全てのゲームオブジェクトをループで更新する
	// これで m_Transform が全オブジェクト分計算される
	for (auto& obj : m_GameObjects)
	{
		// オブジェクトごとに違う動きをさせても良い
		obj->Update(-0.03f);
	}
}

void SceneBase::Draw()
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
		auto pTransform = obj->constantBuffer->GetPtr<SharedStruct::Transform>();
		pTransform->World = obj->GetTransform();
		pTransform->View = DirectX::XMMatrixLookAtRH(m_Camera->GetEyePos(), m_Camera->GetTargetPos(), m_Camera->GetUpward());
		pTransform->Proj = DirectX::XMMatrixPerspectiveFovRH(m_Camera->GetFOV(), m_Camera->GetAspect(), 0.3f, 1000.0f);

		// 更新した定数バッファを GPU にセット
		commandList->SetGraphicsRootConstantBufferView(0, obj->constantBuffer->GetAddress());

		// オブジェクトを描画
		m_Renderer->DrawGameObject(commandList, obj);
	}
}


void SceneBase::SetConstantBuffer()
{
	for (size_t i = 0; i < Engine::FRAME_BUFFER_COUNT; i++)
	{
		constantBuffer[i] = new ConstantBuffer(sizeof(SharedStruct::Transform));
		if (!constantBuffer[i]->IsValid())
		{
			printf("変換行列の登録");
			return ;
		}

		auto ptr = constantBuffer[i]->GetPtr<SharedStruct::Transform>();
		ptr->World = DirectX::XMMatrixIdentity();
		ptr->View = DirectX::XMMatrixLookAtRH(m_Camera->GetEyePos(), m_Camera->GetTargetPos(), m_Camera->GetUpward());
		ptr->Proj = DirectX::XMMatrixPerspectiveFovRH(m_Camera->GetFOV(), m_Camera->GetAspect(), 0.3f, 1000.0f);
	}
}