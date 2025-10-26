#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Core/Components/TriggerComponent.h"
#include "Input/InputDevice.h"
#include "Modules/PublicConst/const_path_pref.h"
#include "Physics/MyCollisionListener.h"
#include "Physics/Component/Rigidbody.h"
#include "Renderer/Loader/PSOLoader.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/DefaultMesh/DefaultMeshes.h"
#include "Scene/GameObject/Loader/GameObjectLoader.h"
#include "Scene/Time/TimeManager.h"

using namespace DirectX;
using json = nlohmann::json;

bool DefaultScene::Init(std::string go_file_path)
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

	InitializeGameObject(go_file_path);

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

	// SceneCameraの更新
	m_Camera->Update(delta_time);
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

void DefaultScene::InitializeGameObject(std::string file_path)
{
	// ゲームオブジェクトの読み込み
	m_GameObjects = GameObjectLoader::load_from_file(file_path);

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

bool DefaultScene::serialize_game_objects(const std::string& go_file_path)
{
	json sceneJson;
    sceneJson["scene_name"] = "tmp";

    json gameObjectsJson = json::array();

    for (const auto& obj : this->get_game_objects())
    {
        json objJson;
        objJson["name"] = obj->get_name();

        // Transform
        {
            auto pos = obj->get_position();
            auto scale = obj->get_scale();
            auto rot = obj->get_rotation();

            objJson["position"] = { pos.x, pos.y, pos.z };
            objJson["scale"]    = { scale.x, scale.y, scale.z };
            objJson["rotation"] = { rot.x, rot.y, rot.z };
        }

        // Components
        json componentsJson = json::array();
        for (const auto& comp : obj->components)
        {
            json compJson;
            const std::string type = comp->get_type();

            compJson["type"] = type;

            if (type == "MeshRenderer")
            {
            	auto meshRenderer = std::dynamic_pointer_cast<MeshRenderer>(comp);
                compJson["model_name"] = meshRenderer->model_name;
            }
            else if (type == "Rigidbody")
            {
            	auto rigidbody = std::dynamic_pointer_cast<Rigidbody>(comp);
                compJson["isGravityEnabled"] = rigidbody->is_gravity_enabled();
            	auto body = rigidbody->get_body_type();
            	compJson["bodyType"] = (body == reactphysics3d::BodyType::DYNAMIC) ? "DYNAMIC" :
									   (body == reactphysics3d::BodyType::KINEMATIC) ? "KINEMATIC" :
									   "STATIC";

                json colliderJson;
                const auto collider = rigidbody->get_collider_object();

            	// TODO: 複数のColliderに対応する場合はここを修正

            	// コライダーの形状を取得
            	auto convex_shape = collider->m_ColliderObjects->get_shape();
            	auto shapeType = convex_shape->getName();

                if (shapeType == reactphysics3d::CollisionShapeName::CAPSULE)
                {
                	// 球形コライダーの場合
                	colliderJson["shape"] = "Capsule";

                	// SphereShapeにキャストして半径を取得
                	auto shape = static_cast<reactphysics3d::CapsuleShape*>(convex_shape);
                    colliderJson["height"] = shape->getHeight();
                    colliderJson["radius"] = shape->getRadius();
                }
                else if (shapeType == reactphysics3d::CollisionShapeName::BOX)
                {
                	// 箱形コライダーの場合
					colliderJson["shape"] = "Box";

                	// BoxShapeにキャストして半径を取得
                	auto shape = static_cast<reactphysics3d::BoxShape*>(convex_shape);
                	auto halfExtents = shape->getHalfExtents();
                    colliderJson["direction"] = {
                        halfExtents.x,
                        halfExtents.y,
                        halfExtents.z
                    };
                }

                compJson["isCollider"] = colliderJson;
            }
            else if (type == "Trigger")
            {
				auto triggerComp = std::dynamic_pointer_cast<TriggerComponent>(comp);

				if (triggerComp->Condition != nullptr)
				{
					compJson["Trigger"]["name"] = triggerComp->Condition->GetName();	
				}

            	for (auto & param : triggerComp->Actions)
            	{
            		// TODO:上書きしてるだけ説ある
            		compJson["Action"]["name"] = param->GetName();
            	}
            }

            componentsJson.push_back(compJson);
        }

        objJson["components"] = componentsJson;

        gameObjectsJson.push_back(objJson);
    }

    sceneJson["gameObjects"] = gameObjectsJson;

    auto result_json = sceneJson.dump(4);  // 4はインデント幅

	// ファイルストリームを開く
	std::ofstream ofs(go_file_path);

	// ファイルが開けたか確認
	if (!ofs.is_open())
	{
		// エラー処理（標準エラー出力など）
		std::cerr << "Error: Failed to open file for writing: " << go_file_path << std::endl;
		return false; // 失敗した場合は空文字列を返す
	}

	// ファイルにJSON文字列を書き込む
	ofs << result_json;

	// ファイルを閉じる
	ofs.close();

	// 関数のシグネチャに従い、生成したJSON文字列を返す
	return true;
}

void DefaultScene::deserialize_game_objects(const std::string& go_file_path)
{
	
}
