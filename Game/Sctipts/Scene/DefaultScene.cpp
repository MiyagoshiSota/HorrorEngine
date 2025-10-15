#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Physics/Component/Rigidbody.h"
#include "Renderer/Graphics/RootSignatureBuilder.h"
#include "Renderer/Loader/PSOLoader.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/DefaultMesh/DefaultMeshes.h"
#include "Scene/GameObject/Loader/GameObjectLoader.h"

using namespace DirectX;

bool DefaultScene::Init()
{
	// カメラの初期化
	m_Camera = std::make_unique<SceneCamera>();
	m_Camera->Init();

	// PipelineStateの初期化
	m_PipelineStateManager = std::make_unique<PipelineStateManager>();

	// PipelineManagerの初期化
	m_PipelineManager = std::make_unique<DefaultPipelineManager>();

	// PhysicsWorldの初期化
	RebuidPhysicsWorld();

	// AudioManagerの初期化
	m_AudioManager = std::make_shared<AudioManager>();
	m_AudioManager->init();

	// Primitiveな3Dモデルの初期化
	CreatePrimitiveObjects();

	printf("PSOの生成");

	PSOLoader::load_from_file("assets/pos_definitions.json", m_PipelineStateManager);

	printf("シーンの初期化に成功\n");

	InitializeGameObject();

	printf("ゲームオブジェクトの初期化を設定");

	g_lastFrameTime = std::chrono::steady_clock::now();

	return true;
}

void DefaultScene::Update(float delta_time)
{
	ISceneBase::Update(delta_time);

	m_physicsWorld->update(delta_time);
}	

void DefaultScene::EditorUpdate()
{
	ISceneBase::EditorUpdate();
}

void DefaultScene::Draw()
{
	m_PipelineManager->Execute();
}

void DefaultScene::shutdown()
{
	// AudioManagerの終了処理
	if (m_AudioManager) {
		m_AudioManager->shutdown();
	}
}

void DefaultScene::RebuidPhysicsWorld()
{
	if (m_physicsWorld) {
		physics_common.destroyPhysicsWorld(m_physicsWorld);
	}
	m_physicsWorld = physics_common.createPhysicsWorld();
	m_physicsWorld->setGravity(reactphysics3d::Vector3(0, -9.81f, 0));
}

void DefaultScene::InitializeGameObject()
{
	// ゲームオブジェクトの読み込み
	m_GameObjects = GameObjectLoader::load_from_file("assets/scene.json");

	// リソースの確保
	for (auto& obj : m_GameObjects)
	{
		m_SceneResourceManager->initialize_gpu_resources_for(obj);
	}

	// ゲームオブジェクトのInitを実行
	for (auto& obj : m_GameObjects)
	{
		obj->init();
	}
}

void DefaultScene::CreatePrimitiveObjects()
{
	// 平面
	auto quad_model = std::make_shared<Model>();
	quad_model->m_InputMesh.push_back(DefaultMeshes::create_quad());
	m_models["primitive/quad"] = quad_model;

	// 立方体
	auto cube_model = std::make_shared<Model>();
	cube_model->m_InputMesh.push_back(DefaultMeshes::create_cube());
	m_models["primitive/cube"] = cube_model;
}