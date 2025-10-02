#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include "../GameObject/DefaultGameObject.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"

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

		m_GameObjects.push_back(gameObj);
	}

	// それぞれのオブジェクトに初期位置を設定
	for (int i = 0; i < gameobjectCount; i++)
	{
		m_GameObjects[i]->SetPosition(i*10, 0, 0);
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

	// PipelineManagerの初期化
	m_PipelineManager = std::make_unique<DefaultPipelineManager>();

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
	m_PipelineManager->Execute();
}