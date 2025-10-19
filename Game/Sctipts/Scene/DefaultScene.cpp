#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Modules/PublicConst/const_path_pref.h"
#include "Physics/MyCollisionListener.h"
#include "Physics/Component/Rigidbody.h"
#include "Renderer/Loader/PSOLoader.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/DefaultMesh/DefaultMeshes.h"
#include "Scene/GameObject/Loader/GameObjectLoader.h"
#include "Scene/Time/TimeManager.h"

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
	// DefaultPipelineManager型にキャスト
	// NOTE:DefaultPipelineManagerのメソッドを使用可能にするため
	m_default_pipeline_manager = std::dynamic_pointer_cast<DefaultPipelineManager>(get_pipeline_manager());
	
	// PhysicsWorldの初期化
	RebuidPhysicsWorld();

	// AudioManagerの初期化
	m_AudioManager = std::make_shared<AudioManager>();
	m_AudioManager->init();

	// Primitiveな3Dモデルの初期化
	CreatePrimitiveObjects();

	// TimeManagerの初期化
	m_TimeManager = std::make_unique<TimeManager>();
	m_TimeManager->init();

	// LightingManagerの初期化
	m_LightingManager = std::make_unique<LightingManager>();
	m_LightingManager->init();
	// TEST:Lightを環境光とポイントライトを一個だけ追加
	m_LightingManager->add_directional_light(
		LightType::Directional,
		DirectX::XMFLOAT3(0.4f, 0.4f, 0.4f),
		2.0f,
		DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f)
	);
	m_LightingManager->add_point_light(
		LightType::Point,
		DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f),
		3.0f,
		DirectX::XMFLOAT3(0.0f, -5.0f, 0.0f),
		20.0f,
		0.1f
	);

	printf("PSOの生成");

	PSOLoader::load_from_file(const_path_pref::PSO_JsonPath, m_PipelineStateManager);

	printf("シーンの初期化に成功\n");

	InitializeGameObject();

	printf("ゲームオブジェクトの初期化を設定");

	g_lastFrameTime = std::chrono::steady_clock::now();

	return true;
}

void DefaultScene::Update(float delta_time)
{
	ISceneBase::Update(delta_time);

	// ポストプロセスマネージャの更新
	m_default_pipeline_manager->get_post_process_manager()->Update(delta_time);

	// 物理演算の更新
	m_physicsWorld->update(delta_time);

	m_LightingManager->update_constant_buffer();
}	

void DefaultScene::EditorUpdate(float delta_time)
{
	ISceneBase::EditorUpdate(delta_time);

	// ポストプロセスマネージャの更新
	m_default_pipeline_manager->get_post_process_manager()->Update(delta_time);

	m_LightingManager->update_constant_buffer();
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

	m_CollisionListener = std::make_shared<MyCollisionListener>();
	m_physicsWorld->setEventListener(m_CollisionListener.get());
}

void DefaultScene::InitializeGameObject()
{
	// ゲームオブジェクトの読み込み
	m_GameObjects = GameObjectLoader::load_from_file(const_path_pref::GameObjectPath);

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