#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Core/Components/TriggerComponent.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Modules/PublicConst/ConstPathPref.h"
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

bool DefaultScene::Init(std::string goFilePath)
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
    m_defaultPipelineManager = std::dynamic_pointer_cast<DefaultPipelineManager>(GetPipelineManager());

    // PhysicsWorldの初期化
    RebuidPhysicsWorld();

    // AudioManagerの初期化
    m_AudioManager = std::make_shared<AudioManager>();
    m_AudioManager->Init();

    // TimeManagerの初期化
    m_TimeManager = std::make_unique<TimeManager>();
    m_TimeManager->Init();

    // LightingManagerの初期化
    m_LightingManager = std::make_unique<LightingManager>();
    m_LightingManager->Init();
    // TEST:Lightを環境光とポイントライトを一個だけ追加
    m_LightingManager->AddDirectionalLight(
        LightType::Directional,
        DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f),
        5.0f,
        DirectX::XMFLOAT3(-1.0f, -1.0f, 0.5f)
    );
    //m_LightingManager->AddPointLight(
    //    LightType::Point,
    //    DirectX::XMFLOAT3(1.0f, .0f, .0f),
    //    3.0f,
    //    DirectX::XMFLOAT3(0.0f, 5.0f, 0.0f),
    //    500.0f,
    //    0.005f
    //);
    //m_LightingManager->AddSpotLight(
    //    LightType::Spot,
    //    DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f),
    //    5.0f,
    //    DirectX::XMFLOAT3(0.0f, 10.0f, 0.0f),
    //    DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f),
    //    0.7f, 1.0f,100, 0.01f
    //);

    printf("PSOの生成\n");

    PSOLoader::LoadFromFile(ConstPathPref::kPSOJsonPath, m_PipelineStateManager);

    // SkyboxManagerの初期化
    m_skyboxManager = std::make_unique<SkyboxManager>();
    
    // Skyboxキューブマップの設定
    // TODO: パスを設定ファイルから読み込むように変更する
    if (m_defaultPipelineManager && m_skyboxManager)
    {
       m_skyboxManager->LoadAndSetup(L"Assets/skybox.dds", m_defaultPipelineManager->GetSkyboxPass());
    }

    printf("シーンの初期化に成功\n");

    InitializeGameObject(goFilePath);

    printf("ゲームオブジェクトの初期化を設定\n");

    g_lastFrameTime = std::chrono::steady_clock::now();

    return true;
}

void DefaultScene::Update(float deltaTime)
{
    ISceneBase::Update(deltaTime);

    // ポストプロセスマネージャの更新
    m_defaultPipelineManager->GetPostProcessManager()->Update(deltaTime);

    GetPhysicsWorld()->setIsDebugRenderingEnabled(true);
    
    // 物理演算の更新
    m_physicsWorld->update(deltaTime);

    m_LightingManager->UpdateConstantBuffer();
}

void DefaultScene::EditorUpdate(float deltaTime)
{
    ISceneBase::EditorUpdate(deltaTime);

    // ポストプロセスマネージャの更新
    m_defaultPipelineManager->GetPostProcessManager()->Update(deltaTime);

    m_LightingManager->UpdateConstantBuffer();

    // SceneCameraの更新
    m_Camera->Update(deltaTime);

    GetPhysicsWorld()->setIsDebugRenderingEnabled(true);
}

void DefaultScene::Draw()
{
    m_PipelineManager->Execute();
}

void DefaultScene::Shutdown()
{
	// AudioManagerの終了処理
	if (m_AudioManager)
	{
		m_AudioManager->Shutdown();
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

void DefaultScene::InitializeGameObject(std::string filePath)
{
    // ゲームオブジェクトの読み込み
    m_GameObjects = GameObjectLoader::LoadFromFile(filePath);

    // ゲームオブジェクトのInitを実行
    for (auto& obj : m_GameObjects)
    {
        obj->Init();
    }
}

// TODO:SceneClassが持つべきか怪しい
bool DefaultScene::SerializeGameObjects(const std::string& goFilePath)
{
    json sceneJson;
    sceneJson[ConstGameObjectSaveParamPref::kSceneName] = goFilePath; // or maybe scene name?

    json gameObjectsJson = json::array();

    for (const auto& obj : this->GetGameObjects())
    {
        json objJson;
        objJson[ConstGameObjectSaveParamPref::kGameObjectName] = obj->GetName();

        // Transform
        {
            auto pos = obj->GetPosition();
            auto scale = obj->GetScale();
            auto rot = obj->GetRotation();

            objJson[ConstGameObjectSaveParamPref::kTransformPosition] = {pos.x, pos.y, pos.z};
            objJson[ConstGameObjectSaveParamPref::kTransformScale] = {scale.x, scale.y, scale.z};
            objJson[ConstGameObjectSaveParamPref::kTransformRotation] = {rot.x, rot.y, rot.z};
        }

        // Components
        json componentsJson = json::array();
        for (const auto& comp : obj->components) // obj->m_components かもしれない
        {
            if (!comp) continue; // nullptr チェック

            json compJson;
            const std::string type = comp->GetType();

            compJson[ConstGameObjectSaveParamPref::kComponentType] = type;

            if (type == ConstGameObjectSaveParamPref::kComponentMeshRenderer)
            {
                const auto* meshRenderer = dynamic_cast<MeshRenderer*>(comp.get());
                if (meshRenderer)
                {
                    // キャスト成功確認
                    compJson["model_name"] = meshRenderer->model_name; // "model_name" は定数クラスに追加しても良い
                }
            }
            else if (type == ConstGameObjectSaveParamPref::kComponentRigidbody)
            {
                auto* rigidbody = dynamic_cast<Rigidbody*>(comp.get());
                if (rigidbody && rigidbody->GetRigidbody())
                {
                    // キャスト成功 & RigidBodyが存在するか確認
                    compJson[ConstGameObjectSaveParamPref::kRigidbodyIsGravityEnabled] = rigidbody->
                        IsGravityEnabled();
                    auto body = rigidbody->GetBodyType();
                    compJson[ConstGameObjectSaveParamPref::kRigidbodyBodyType] =
                        (body == reactphysics3d::BodyType::DYNAMIC)
                            ? "DYNAMIC"
                            : (body == reactphysics3d::BodyType::KINEMATIC)
                            ? "KINEMATIC"
                            : "STATIC";

                    // Collider情報
                    const auto engineCollider = rigidbody->GetColliderObject(); // shared_ptr<EngineCollider> を取得
                    // EngineCollider と reactphysics3d::Collider の両方が存在するか確認
                    if (engineCollider && engineCollider->GetCollider())
                    {
                        json colliderJson;
                        reactphysics3d::Collider* rp3dCollider = engineCollider->GetCollider();
                        EngineCollider::ShapeType shapeType = engineCollider->GetShapeType();

                        if (shapeType == EngineCollider::ShapeType::CAPSULE)
                        {
                            colliderJson[ConstGameObjectSaveParamPref::kColliderShape] =
                                ConstGameObjectSaveParamPref::kColliderShapeCapsule;
                            colliderJson[ConstGameObjectSaveParamPref::kColliderCapsuleHeight] = engineCollider->
                                GetCapsuleHeight();
                            colliderJson[ConstGameObjectSaveParamPref::kColliderCapsuleRadius] = engineCollider->
                                GetCapsuleRadius();
                        }
                        else if (shapeType == EngineCollider::ShapeType::BOX)
                        {
                            colliderJson[ConstGameObjectSaveParamPref::kColliderShape] =
                                ConstGameObjectSaveParamPref::kColliderShapeBox;
                            reactphysics3d::Vector3 halfExtents = engineCollider->GetBoxHalfExtents();
                            colliderJson[ConstGameObjectSaveParamPref::kColliderBoxHalfExtents] = {
                                halfExtents.x, halfExtents.y, halfExtents.z
                            };
                        }
                        else if (shapeType == EngineCollider::ShapeType::SPHERE) // SPHERE を追加
                        {
                            colliderJson[ConstGameObjectSaveParamPref::kColliderShape] =
                                ConstGameObjectSaveParamPref::kColliderShapeSphere;
                            colliderJson[ConstGameObjectSaveParamPref::kColliderSphereRadius] = engineCollider->
                                GetSphereRadius();
                        }

                        // Trigger設定も保存
                        colliderJson[ConstGameObjectSaveParamPref::kColliderIsTrigger] = rp3dCollider->
                            getIsTrigger();

                        // Colliderのローカルトランスフォームも保存 (必要なら)
                        // reactphysics3d::Transform localTransform = rp3dCollider->getLocalToBodyTransform();
                        // ... localTransform をJSONに保存する処理 ...

                        compJson[ConstGameObjectSaveParamPref::kRigidbodyCollider] = colliderJson;
                    }
                }
            }
            else if (type == ConstGameObjectSaveParamPref::kComponentTrigger)
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
                        conditionJson[ConstGameObjectSaveParamPref::kTriggerConditionName] = trigger_comp->Condition->
                            GetName();
                        // TODO: Condition にパラメータがあればここに追加
                        compJson[ConstGameObjectSaveParamPref::kTriggerCondition] = conditionJson;
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
                                actionJson[ConstGameObjectSaveParamPref::kTriggerActionName] = action->GetName();

                                // TODO: Action ごとのパラメータを保存する処理
                                if (action->GetName() == "StartWork")
                                {
                                    auto* startWorkAction = dynamic_cast<StartWorkReward*>(action.get());
                                    if (startWorkAction && startWorkAction->GetWork())
                                    {
                                        actionJson["workName"] = startWorkAction->GetWork()->m_name;
                                    }
                                }
                                actionsJson.push_back(actionJson);
                            }
                        }
                        // "Reward" ではなく複数のActionを格納するキー名を使用
                        compJson[ConstGameObjectSaveParamPref::kTriggerActions] = actionsJson;
                    }
                }
            }
            else if (type == ConstGameObjectSaveParamPref::kComponentPlayerController)
            {
                auto* playerController = dynamic_cast<PLayerController*>(comp.get());
                if (playerController)
                {
                    // キャスト成功確認
                    compJson[ConstGameObjectSaveParamPref::kPlayerControllerMoveSpeed] = playerController->
                        GetMoveSpeed();
                }
            }

            componentsJson.push_back(compJson);
        }

        objJson[ConstGameObjectSaveParamPref::kComponents] = componentsJson;
        gameObjectsJson.push_back(objJson);
    }

    sceneJson[ConstGameObjectSaveParamPref::kGameObjects] = gameObjectsJson;

    // JSONを整形して文字列化
    std::string result_json = sceneJson.dump(4); // 4はインデント幅

    // ファイルストリームを開く
    std::ofstream ofs(goFilePath);

    // ファイルが開けたか確認
    if (!ofs.is_open())
    {
        std::cerr << "Error: Failed to open file for writing: " << goFilePath << std::endl;
        return false;
    }

    // ファイルにJSON文字列を書き込む
    ofs << result_json;

    // ファイルを閉じる
    ofs.close();

    return true; // 成功
}

// TODO:SceneClassが持つべきか怪しい
void DefaultScene::DeserializeGameObjects(const std::string& goFilePath)
{
}
