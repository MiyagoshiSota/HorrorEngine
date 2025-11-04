#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Core/Components/TriggerComponent.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Modules/PublicConst/const_path_pref.h"
#include "Physics/MyCollisionListener.h"
#include "Physics/Component/Rigidbody.h"
#include "Renderer/Loader/PSOLoader.h"
#include "Scene/Character/Player/PlayerController.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
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
        DirectX::XMFLOAT3(0.f, 0.f, 0.f),
        1.0f,
        DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f)
    );
    m_LightingManager->add_point_light(
        LightType::Point,
        DirectX::XMFLOAT3(1.0f, .0f, .0f),
        3.0f,
        DirectX::XMFLOAT3(0.0f, 5.0f, 0.0f),
        500.0f,
        0.005f
    );
    m_LightingManager->add_spot_light(
        LightType::Spot,
        DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f),
        5.0f,
        DirectX::XMFLOAT3(0.0f, 10.0f, 0.0f),
        DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f),
        0.7f, 1.0f,100, 0.01f
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

    get_physics_world()->setIsDebugRenderingEnabled(true);
    
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

    get_physics_world()->setIsDebugRenderingEnabled(true);
}

void DefaultScene::Draw()
{
    m_PipelineManager->Execute();
}

void DefaultScene::shutdown()
{
    // AudioManagerの終了処理
    if (m_AudioManager)
    {
        m_AudioManager->shutdown();
    }
}

void DefaultScene::RebuidPhysicsWorld()
{
    if (m_physicsWorld)
    {
        physics_common.destroyPhysicsWorld(m_physicsWorld);
    }
    m_physicsWorld = physics_common.createPhysicsWorld();
    m_physicsWorld->setGravity(reactphysics3d::Vector3(0, -9.81f, 0));

    m_CollisionListener = std::make_shared<MyCollisionListener>();
    m_physicsWorld->setEventListener(m_CollisionListener.get());

    m_physicsWorld->setIsDebugRenderingEnabled(true);
}

void DefaultScene::InitializeGameObject(std::string file_path)
{
    // ゲームオブジェクトの読み込み
    m_GameObjects = GameObjectLoader::load_from_file(file_path);

    // リソースの確保
    for (auto& obj : m_GameObjects)
    {
        SceneResourceManager::GetInstance().initialize_gpu_resources_for(obj);
    }

    // ゲームオブジェクトのInitを実行
    for (auto& obj : m_GameObjects)
    {
        obj->init();
    }
}

// TODO:SceneClassが持つべきか怪しい
bool DefaultScene::serialize_game_objects(const std::string& go_file_path)
{
    json sceneJson;
    sceneJson[const_gameobject_save_param_pref::SceneName] = go_file_path; // or maybe scene name?

    json gameObjectsJson = json::array();

    for (const auto& obj : this->get_game_objects())
    {
        json objJson;
        objJson[const_gameobject_save_param_pref::GameObjectName] = obj->get_name();

        // Transform
        {
            auto pos = obj->get_position();
            auto scale = obj->get_scale();
            auto rot = obj->get_rotation();

            objJson[const_gameobject_save_param_pref::TransformPosition] = {pos.x, pos.y, pos.z};
            objJson[const_gameobject_save_param_pref::TransformScale] = {scale.x, scale.y, scale.z};
            objJson[const_gameobject_save_param_pref::TransformRotation] = {rot.x, rot.y, rot.z};
        }

        // Components
        json componentsJson = json::array();
        for (const auto& comp : obj->components) // obj->m_components かもしれない
        {
            if (!comp) continue; // nullptr チェック

            json compJson;
            const std::string type = comp->get_type();

            compJson[const_gameobject_save_param_pref::ComponentType] = type;

            if (type == const_gameobject_save_param_pref::ComponentMeshRenderer)
            {
                const auto* meshRenderer = dynamic_cast<MeshRenderer*>(comp.get());
                if (meshRenderer)
                {
                    // キャスト成功確認
                    compJson["model_name"] = meshRenderer->model_name; // "model_name" は定数クラスに追加しても良い
                }
            }
            else if (type == const_gameobject_save_param_pref::ComponentRigidbody)
            {
                auto* rigidbody = dynamic_cast<Rigidbody*>(comp.get());
                if (rigidbody && rigidbody->get_rigidbody())
                {
                    // キャスト成功 & RigidBodyが存在するか確認
                    compJson[const_gameobject_save_param_pref::RigidbodyIsGravityEnabled] = rigidbody->
                        is_gravity_enabled();
                    auto body = rigidbody->get_body_type();
                    compJson[const_gameobject_save_param_pref::RigidbodyBodyType] =
                        (body == reactphysics3d::BodyType::DYNAMIC)
                            ? "DYNAMIC"
                            : (body == reactphysics3d::BodyType::KINEMATIC)
                            ? "KINEMATIC"
                            : "STATIC";

                    // Collider情報
                    const auto engineCollider = rigidbody->get_collider_object(); // shared_ptr<engine_collider> を取得
                    // engine_collider と reactphysics3d::Collider の両方が存在するか確認
                    if (engineCollider && engineCollider->get_collider())
                    {
                        json colliderJson;
                        reactphysics3d::Collider* rp3dCollider = engineCollider->get_collider();
                        engine_collider::ShapeType shapeType = engineCollider->get_shape_type();

                        if (shapeType == engine_collider::ShapeType::CAPSULE)
                        {
                            colliderJson[const_gameobject_save_param_pref::ColliderShape] =
                                const_gameobject_save_param_pref::ColliderShapeCapsule;
                            colliderJson[const_gameobject_save_param_pref::ColliderCapsuleHeight] = engineCollider->
                                get_capsule_height();
                            colliderJson[const_gameobject_save_param_pref::ColliderCapsuleRadius] = engineCollider->
                                get_capsule_radius();
                        }
                        else if (shapeType == engine_collider::ShapeType::BOX)
                        {
                            colliderJson[const_gameobject_save_param_pref::ColliderShape] =
                                const_gameobject_save_param_pref::ColliderShapeBox;
                            reactphysics3d::Vector3 halfExtents = engineCollider->get_box_half_extents();
                            colliderJson[const_gameobject_save_param_pref::ColliderBoxHalfExtents] = {
                                halfExtents.x, halfExtents.y, halfExtents.z
                            };
                        }
                        else if (shapeType == engine_collider::ShapeType::SPHERE) // SPHERE を追加
                        {
                            colliderJson[const_gameobject_save_param_pref::ColliderShape] =
                                const_gameobject_save_param_pref::ColliderShapeSphere;
                            colliderJson[const_gameobject_save_param_pref::ColliderSphereRadius] = engineCollider->
                                get_sphere_radius();
                        }

                        // Trigger設定も保存
                        colliderJson[const_gameobject_save_param_pref::ColliderIsTrigger] = rp3dCollider->
                            getIsTrigger();

                        // Colliderのローカルトランスフォームも保存 (必要なら)
                        // reactphysics3d::Transform localTransform = rp3dCollider->getLocalToBodyTransform();
                        // ... localTransform をJSONに保存する処理 ...

                        compJson[const_gameobject_save_param_pref::RigidbodyCollider] = colliderJson;
                    }
                }
            }
            else if (type == const_gameobject_save_param_pref::ComponentTrigger)
            {
                auto* trigger_comp = dynamic_cast<TriggerComponent*>(comp.get());
                if (trigger_comp)
                {
                    // キャスト成功確認
                    // Condition
                    if (trigger_comp->Condition != nullptr)
                    {
                        // JSON オブジェクトを作成して Condition の情報を格納
                        json conditionJson;
                        conditionJson[const_gameobject_save_param_pref::TriggerConditionName] = trigger_comp->Condition->
                            GetName();
                        // TODO: Condition にパラメータがあればここに追加
                        compJson[const_gameobject_save_param_pref::TriggerCondition] = conditionJson;
                    }

                    // Reward
                    if (!trigger_comp->Actions.empty())
                    {
                        json actionsJson = json::array();
                        for (const auto& action : trigger_comp->Actions)
                        {
                            if (action)
                            {
                                json actionJson;
                                actionJson[const_gameobject_save_param_pref::TriggerActionName] = action->GetName();

                                // TODO: Action ごとのパラメータを保存する処理
                                if (action->GetName() == "StartWork")
                                {
                                    auto* startWorkAction = dynamic_cast<StartWorkReward*>(action.get());
                                    if (startWorkAction && startWorkAction->get_work())
                                    {
                                        actionJson["workName"] = startWorkAction->get_work()->m_name;
                                    }
                                }
                                actionsJson.push_back(actionJson);
                            }
                        }
                        // "Reward" ではなく複数のActionを格納するキー名を使用
                        compJson[const_gameobject_save_param_pref::TriggerActions] = actionsJson;
                    }
                }
            }
            else if (type == const_gameobject_save_param_pref::ComponentPlayerController)
            {
                auto* playerController = dynamic_cast<PLayerController*>(comp.get());
                if (playerController)
                {
                    // キャスト成功確認
                    compJson[const_gameobject_save_param_pref::PlayerControllerMoveSpeed] = playerController->
                        get_move_speed();
                }
            }

            componentsJson.push_back(compJson);
        }

        objJson[const_gameobject_save_param_pref::Components] = componentsJson;
        gameObjectsJson.push_back(objJson);
    }

    sceneJson[const_gameobject_save_param_pref::GameObjects] = gameObjectsJson;

    // JSONを整形して文字列化
    std::string result_json = sceneJson.dump(4); // 4はインデント幅

    // ファイルストリームを開く
    std::ofstream ofs(go_file_path);

    // ファイルが開けたか確認
    if (!ofs.is_open())
    {
        std::cerr << "Error: Failed to open file for writing: " << go_file_path << std::endl;
        return false;
    }

    // ファイルにJSON文字列を書き込む
    ofs << result_json;

    // ファイルを閉じる
    ofs.close();

    return true; // 成功
}

// TODO:SceneClassが持つべきか怪しい
void DefaultScene::deserialize_game_objects(const std::string& go_file_path)
{
}
