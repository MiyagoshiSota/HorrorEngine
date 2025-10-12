#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Physics/Component/Rigidbody.h"
#include "Renderer/Graphics/RootSignatureBuilder.h"
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
	auto name = "DefaultPipelinePass";

	// RootSignatureの生成・初期化
	auto builder = std::make_shared<RootSignatureBuilder>();

	// ディスクリプタレンジを定義
	CD3DX12_DESCRIPTOR_RANGE tableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// スタティックサンプラーを定義
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	// フラグを定義
	auto flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// ビルダーを使ってルートシグネチャを定義
	builder->add_constant_buffer_view(0) // b0
		.add_descriptor_table(1, &tableRange)
		.add_static_sampler(sampler)
		.set_flags(flags);

	// POSの生成・初期化
	m_PipelineStateManager->new_create(L"../x64/Debug/SimpleVS.cso", L"../x64/Debug/SimplePS.cso",true, true, builder, name);
	
	// PipelineManagerの初期化
	m_PipelineManager = std::make_unique<DefaultPipelineManager>();

	// PhysicsWorldの初期化
	RebuidPhysicsWorld();

	// AudioManagerの初期化
	m_AudioManager = std::make_shared<AudioManager>();
	m_AudioManager->init();

	// Primitiveな3Dモデルの初期化
	CreatePrimitiveObjects();

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