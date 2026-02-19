#include "DefaultScene.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include  "../Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Core/Components/TriggerComponent.h"
#include "Core/Components/Reward/PlaySoundReward.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Core/Components/Work/WorkManager.h"
#include "Modules/PublicConst/ConstPathPref.h"
#include "Physics/MyCollisionListener.h"
#include "Physics/Component/Rigidbody.h"
#include "Renderer/Loader/PSOLoader.h"
#include "Scene/Character/Player/PlayerController.h"
#include "Scene/Character/Player/Player.h"
#include "Scene/Camera/PlayModeCameraConfig.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Loader/GameObjectLoader.h"
#include "Scene/Time/TimeManager.h"
#include "Input/InputDevice.h"
#include <nlohmann/json.hpp>
#include <fstream>

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

    // Debug用のライトを追加
    m_LightingManager->AddDirectionalLight(
        LightType::Directional,
        DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f),
        8.0f,
        DirectX::XMFLOAT3(-1.0f, -1.0f, 0.5f)
    );
    //m_LightingManager->AddPointLight(
    //   LightType::Point,
    //   DirectX::XMFLOAT3(1.0f, .0f, .0f),
    //   3.0f,
    //   DirectX::XMFLOAT3(0.0f, 5.0f, 0.0f),
    //   500.0f,
    //   0.005f
    //);
    //m_LightingManager->AddSpotLight(
    //   LightType::Spot,
    //   DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f),
    //   5.0f,
    //   DirectX::XMFLOAT3(0.0f, 10.0f, 0.0f),
    //   DirectX::XMFLOAT3(0.0f, -1.0f, 0.0f),
    //   0.7f, 1.0f,100, 0.01f
    //);

    printf("PSOの生成\n");

    // PSOの読み込み
    PSOLoader::LoadFromFile(ConstPathPref::kPSOJsonPath, m_PipelineStateManager);

    // SkyboxManagerの初期化
    m_skyboxManager = std::make_unique<SkyboxManager>();
    
    // Skyboxキューブマップの設定
    if (m_defaultPipelineManager && m_skyboxManager)
    {
       m_skyboxManager->LoadAndSetup(L"Assets/skybox.dds", m_defaultPipelineManager->GetSkyboxPass());
    }

    // Ray Traced Shadow Managerの初期化
    if (g_Engine && g_Engine->IsDxrSupported())
    {
        m_rayTracedShadowManager = std::make_unique<RayTracedShadowManager>();
        Microsoft::WRL::ComPtr<ID3D12Device5> device5;
        if (SUCCEEDED(g_Engine->Device()->QueryInterface(IID_PPV_ARGS(&device5))))
        {
            // カメラ視点のため、解像度は画面サイズに合わせる
            if (!m_rayTracedShadowManager->Init(device5.Get(), kWindowWidth, kWindowHeight))
            {
                m_rayTracedShadowManager.reset();
            }
        }

        // RTAO Managerの初期化（同一DXRデバイス・解像度でTLASはShadowと共有）
        m_rayTracedAOManager = std::make_unique<RayTracedAOManager>();
        if (!device5 || !m_rayTracedAOManager->Init(device5.Get(), kWindowWidth, kWindowHeight))
        {
            m_rayTracedAOManager.reset();
        }

        // RTGI Managerの初期化（TLASはShadowと共有）
        m_rayTracedGIManager = std::make_unique<RayTracedGIManager>();
        if (!device5 || !m_rayTracedGIManager->Init(device5.Get(), kWindowWidth, kWindowHeight))
        {
            m_rayTracedGIManager.reset();
        }

        m_rayTracedReflectionManager = std::make_unique<RayTracedReflectionManager>();
        if (!device5 || !m_rayTracedReflectionManager->Init(device5.Get(), kWindowWidth, kWindowHeight))
        {
            m_rayTracedReflectionManager.reset();
        }
    }

    printf("シーンの初期化に成功\n");

    InitializeGameObject(goFilePath);

    printf("ゲームオブジェクトの初期化を設定\n");

    g_lastFrameTime = std::chrono::steady_clock::now();

    return true;
}

void DefaultScene::Update(float deltaTime)
{
    // 基底クラスの更新
    ISceneBase::Update(deltaTime);

    // プレイヤーの手持ちアイテムの更新
    Player::GetInstance().UpdateHeldItemTransform();

    // ポストプロセスマネージャの更新
    m_defaultPipelineManager->GetPostProcessManager()->Update(deltaTime);

    // 物理演算の更新
    m_physicsWorld->update(deltaTime);

    // ライティングマネージャの更新
    m_LightingManager->UpdateConstantBuffer();

    // デバッグ線を表示するためのフラグを設定
    GetPhysicsWorld()->setIsDebugRenderingEnabled(true);
    auto& debugRenderer = GetPhysicsWorld()->getDebugRenderer();
    debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);

    // PlayMode 用カメラ設定の適用
    ApplyPlayModeCamera(deltaTime);
}

void DefaultScene::EditorUpdate(float deltaTime)
{
    // 基底クラスの更新
    ISceneBase::EditorUpdate(deltaTime);

    // ポストプロセスマネージャの更新
    m_defaultPipelineManager->GetPostProcessManager()->Update(deltaTime);

    // ライティングマネージャの更新
    m_LightingManager->UpdateConstantBuffer();

    // SceneCameraの更新
    m_Camera->Update(deltaTime);

    // デバッグ線を表示するためのフラグを設定
    GetPhysicsWorld()->setIsDebugRenderingEnabled(true);
    auto& debugRenderer = GetPhysicsWorld()->getDebugRenderer();
    debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);
    
    // エディタでは物理シミュレーションを回さず、GameObject の位置で剛体を同期してからデバッグ用プリミティブのみ計算する（重力等がかからない）
    for (const auto& obj : GetGameObjects())
    {
        const auto rb = obj->FindComponent<Rigidbody>();
        if (rb && rb->GetRigidbody())
        {
            const auto pos = obj->GetPosition();
            const reactphysics3d::Transform transform(
                reactphysics3d::Vector3(pos.x, pos.y, pos.z),
                reactphysics3d::Quaternion::identity());
            rb->GetRigidbody()->setTransform(transform);
        }
    }
    debugRenderer.computeDebugRenderingPrimitives(*GetPhysicsWorld());
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

void DefaultScene::ApplyPlayModeCamera(float deltaTime)
{
	auto& config = PlayModeCameraConfig::GetInstance();

	if (config.GetMode() == PlayModeCameraConfig::Mode::Free)
	{
		m_Camera->Update(deltaTime);
		return;
	}

	auto playerGo = Player::GetInstance().GetPlayerGameObject();
	if (!playerGo || !m_Camera)
		return;

	// GameObjectWindow と同じく GameObject の GetPosition / GetRotation から取得
	const XMFLOAT3 pos = playerGo->GetPosition();
	const float posX = pos.x;
	const float posY = pos.y;
	const float posZ = pos.z;

	const XMFLOAT3 rotDeg = playerGo->GetRotation();

	const float sens = config.GetRotationSensitivity() * 0.5f; // Free と同程度
	auto& input = InputDevice::GetInstance();
	const auto mouseDelta = input.GetMouseDelta();

	if (config.GetMode() == PlayModeCameraConfig::Mode::FirstPerson)
	{
		// 1人称: Free と同じく右ドラッグで視点回転
		if (input.IsMouseDown(1))
			config.AddFirstPersonRotation(-mouseDelta.x * sens, -mouseDelta.y * sens);

		const float yaw = config.GetFirstPersonYaw();
		const float pitch = config.GetFirstPersonPitch();
		const float cp = cosf(pitch);
		const float sp = sinf(pitch);
		const float cy = cosf(yaw);
		const float sy = sinf(yaw);
		const float fwdX = sy * cp;
		const float fwdY = sp;
		const float fwdZ = -cy * cp;

		const XMFLOAT3 offset = config.GetFirstPersonEyeOffset();
		const float ex = posX + offset.x;
		const float ey = posY + offset.y;
		const float ez = posZ + offset.z;
		const float dist = 1.0f;
		m_Camera->SetEyePos(ex, ey, ez);
		m_Camera->SetTargetPos(ex + fwdX * dist, ey + fwdY * dist, ez + fwdZ * dist);
		m_Camera->RefreshVectors();
		return;
	}

	if (config.GetMode() == PlayModeCameraConfig::Mode::Follow)
	{
		// 3人称: プレイヤーを中心にオービット（右ドラッグで回転）
		if (input.IsMouseDown(1))
			config.AddFollowOrbitRotation(-mouseDelta.x * sens, mouseDelta.y * sens);

		const float dist = config.GetFollowDistance();
		const float lookAtH = config.GetFollowLookAtHeight();
		const float smooth = config.GetFollowSmoothSpeed();

		const float oy = config.GetFollowOrbitYaw();
		const float op = config.GetFollowOrbitPitch();
		const float co = cosf(op);
		const float so = sinf(op);
		const float cy = cosf(oy);
		const float sy = sinf(oy);
		// 注視点からカメラへの方向（Y-up 球面）
		const float dirX = co * sy;
		const float dirY = so;
		const float dirZ = co * cy;

		const float lookX = posX;
		const float lookY = posY + lookAtH;
		const float lookZ = posZ;
		const float desiredEyeX = lookX + dirX * dist;
		const float desiredEyeY = lookY + dirY * dist;
		const float desiredEyeZ = lookZ + dirZ * dist;

		const float t = 1.0f - expf(-smooth * deltaTime);
		XMVECTOR curEye = m_Camera->GetEyePos();
		XMVECTOR curTgt = m_Camera->GetTargetPos();
		XMVECTOR desEye = XMVectorSet(desiredEyeX, desiredEyeY, desiredEyeZ, 1.0f);
		XMVECTOR desTgt = XMVectorSet(lookX, lookY, lookZ, 1.0f);
		XMVECTOR newEye = XMVectorLerp(curEye, desEye, t);
		XMVECTOR newTgt = XMVectorLerp(curTgt, desTgt, t);

		m_Camera->SetEyePos(XMVectorGetX(newEye), XMVectorGetY(newEye), XMVectorGetZ(newEye));
		m_Camera->SetTargetPos(XMVectorGetX(newTgt), XMVectorGetY(newTgt), XMVectorGetZ(newTgt));
		m_Camera->RefreshVectors();
	}
}

void DefaultScene::InitializeGameObject(std::string filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open scene file: " + filePath);
    }
    const json sceneJson = json::parse(file);

    // ゲームオブジェクトの読み込み
    m_GameObjects = GameObjectLoader::LoadFromJson(sceneJson);

    // ゲームオブジェクトのInitを実行
    for (auto& obj : m_GameObjects)
    {
        obj->Init();
    }

    // Works/Task は Day(Scene) に紐づくため、同一JSONから復元する
    WorkManager::GetInstance().LoadFromSceneJson(sceneJson, m_GameObjects);

    // Trigger の StartWorkReward が持つ workName を Work* に解決する
    TriggerComponent::ResolvePendingWorkReferencesInScene(m_GameObjects);
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
                    compJson["model_name"] = meshRenderer->model_name;
                    const auto& overrides = meshRenderer->GetMaterialColorOverrides();
                    if (!overrides.empty())
                    {
                        json overridesJson = json::array();
                        for (const auto& opt : overrides)
                        {
                            if (opt.has_value())
                                overridesJson.push_back({opt->x, opt->y, opt->z, opt->w});
                            else
                                overridesJson.push_back(nullptr);
                        }
                        compJson[ConstGameObjectSaveParamPref::kMeshRendererMaterialColorOverrides] = overridesJson;
                    }
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

                        // オフセット（剛体中心からのローカル位置）を保存
                        const reactphysics3d::Vector3 offset = engineCollider->GetLocalPosition();
                        colliderJson[ConstGameObjectSaveParamPref::kColliderOffset] = {
                            offset.x, offset.y, offset.z
                        };

                        compJson[ConstGameObjectSaveParamPref::kRigidbodyCollider] = colliderJson;
                    }
                }
            }
            else if (type == ConstGameObjectSaveParamPref::kComponentTrigger)
            {
                auto* trigger_comp = dynamic_cast<TriggerComponent*>(comp.get());
                if (trigger_comp)
                {
                    // Task名（Workflow内での表示名）を保存
                    compJson[ConstGameObjectSaveParamPref::kTriggerTaskName] = trigger_comp->GetTaskName();
                    // Condition
                    if (trigger_comp->Condition != nullptr)
                    {
                        json conditionJson;
                        conditionJson[ConstGameObjectSaveParamPref::kTriggerConditionName] = trigger_comp->Condition->GetName();
                        trigger_comp->Condition->Serialize(conditionJson);
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

                                if (action->GetName() == "StartWorkReward")
                                {
                                    auto* startWorkAction = dynamic_cast<StartWorkReward*>(action.get());
                                    if (startWorkAction && startWorkAction->GetWork())
                                        actionJson["workName"] = startWorkAction->GetWork()->m_name;
                                }
                                if (action->GetName() == "PlaySoundAction")
                                {
                                    auto* playSoundAction = dynamic_cast<PlaySoundReward*>(action.get());
                                    if (playSoundAction)
                                    {
                                        actionJson[ConstGameObjectSaveParamPref::kRewardActionSoundName] = playSoundAction->GetSoundName();
                                        actionJson[ConstGameObjectSaveParamPref::kRewardActionSoundUse3d] = playSoundAction->GetUse3d();
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

    // Works/Task は Day(Scene) に紐づくため、同一JSONに保存する
    sceneJson[ConstGameObjectSaveParamPref::kWorks] =
        WorkManager::GetInstance().SerializeToSceneJson(this->GetGameObjects());

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
