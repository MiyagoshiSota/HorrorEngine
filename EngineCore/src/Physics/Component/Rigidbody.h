#pragma once
#include <set>

#include "Core/App.h"
#include "Modules/PublicConst/ConstGameObjectSaveParamPref.h"
#include "Physics/EngineCollider.h"
#include "Scene/GameObject/Component/Component.h"

class Rigidbody : public Component
{
public:
    Rigidbody() = default;

    ~Rigidbody() override
    {
        // このコンポーネントが破棄される時に、物理ボディも確実に破棄する
        if (m_RigidBody)
        {
            if (g_Scene && g_Scene->GetPhysicsWorld())
            {
                g_Scene->GetPhysicsWorld()->destroyRigidBody(m_RigidBody);
            }
            m_RigidBody = nullptr;
        }
    }

    void Initialize(std::shared_ptr<GameObject> game_object) override
    {
        Component::Initialize(game_object);

        // PhysicsWorldの取得
        auto world = g_Scene->GetPhysicsWorld();

        // PositionをReactPhysics3DのVector3型に変換
        auto fl_pos = gameObject->GetPosition();
        reactphysics3d::Vector3 pos = reactphysics3d::Vector3(fl_pos.x, fl_pos.y, fl_pos.z);

        // ReactPhysics3DのTransform型を作成
        // auto rot = gameObject->get_rotation();
        // reactphysics3d::Quaternion quat = reactphysics3d::Quaternion::fromEulerAngles(
        //     static_cast<reactphysics3d::decimal>(rot.x * static_cast<reactphysics3d::decimal>(M_PI) / 180.0),
        //     static_cast<reactphysics3d::decimal>(rot.y * static_cast<reactphysics3d::decimal>(M_PI) / 180.0),
        //     static_cast<reactphysics3d::decimal>(rot.z * static_cast<reactphysics3d::decimal>(M_PI) / 180.0)
        // );
        auto quat = reactphysics3d::Quaternion::identity();

        reactphysics3d::Transform transform = reactphysics3d::Transform(pos, quat);

        // RigidBodyの作成
        m_RigidBody = world->createRigidBody(transform);

        // 作成した物理ボディに、親であるGameObjectのポインタを保存する
        m_RigidBody->setUserData(gameObject.get());

        m_RigidBody->setMass(10.0f); // デフォルト質量を設定
    }

    void start() override
    {
    }

    void update(float deltaTime) override
    {
        auto v3pos = m_RigidBody->getTransform().getPosition();
        reactphysics3d::Quaternion v3rot = m_RigidBody->getTransform().getOrientation();

        gameObject->SetPosition(v3pos.x, v3pos.y, v3pos.z);

        // オイラー角に変換
        // reactphysics3d::Vector3 eulerAngles = v3rot.getVectorV();
        // gameObject->set_rotation(
        //     static_cast<float>(eulerAngles.x * 180.0 / M_PI),
        //     static_cast<float>(eulerAngles.y * 180.0 / M_PI),
        //     static_cast<float>(eulerAngles.z * 180.0 / M_PI)
        // );
    }

    void Deserialize(const nlohmann::json& jsonData) override
    {
        // RigidBodyがinitializeで作成されている前提
        if (!m_RigidBody)
        {
            printf("Error: Rigidbody::deserialize called before RigidBody was initialized.\n");
            return;
        }

        // --- 定数クラスのエイリアス ---
        using Prefs = ConstGameObjectSaveParamPref;

        // 重力の設定
        if (jsonData.contains(Prefs::kRigidbodyIsGravityEnabled))
        {
            // エラーチェックを追加 (型がboolか)
            if (jsonData[Prefs::kRigidbodyIsGravityEnabled].is_boolean())
            {
                bool data = jsonData[Prefs::kRigidbodyIsGravityEnabled].get<bool>();
                m_RigidBody->enableGravity(data);
            }
            else
            {
                printf("Warning: Rigidbody 'isGravityEnabled' is not a boolean.\n");
            }
        }

        // 剛体のType
        if (jsonData.contains(Prefs::kRigidbodyBodyType))
        {
            // エラーチェックを追加 (型がstringか)
            if (jsonData[Prefs::kRigidbodyBodyType].is_string())
            {
                std::string body_type = jsonData[Prefs::kRigidbodyBodyType].get<std::string>();
                if (body_type == "STATIC") // 文字列比較は定数を使わない方が良い場合もある
                {
                    m_RigidBody->setType(reactphysics3d::BodyType::STATIC);
                }
                else if (body_type == "DYNAMIC")
                {
                    m_RigidBody->setType(reactphysics3d::BodyType::DYNAMIC);
                }
                else if (body_type == "KINEMATIC")
                {
                    m_RigidBody->setType(reactphysics3d::BodyType::KINEMATIC);
                }
            }
            else
            {
                printf("Warning: Rigidbody 'bodyType' is not a string.\n");
            }
        }

        // Colliderの生成
        if (jsonData.contains(Prefs::kRigidbodyCollider)) // キー名を定数に変更
        {
            // エラーチェック (型がobjectか)
            if (!jsonData[Prefs::kRigidbodyCollider].is_object())
            {
                printf("Warning: Rigidbody 'collider' is not an object.\n");
                return;
            }

            const auto& colliderJson = jsonData[Prefs::kRigidbodyCollider]; // 定数を使用

            if (colliderJson.contains(Prefs::kColliderShape) && colliderJson[Prefs::kColliderShape].is_string())
            {
                m_collider = std::make_shared<EngineCollider>();
                std::string shapeType = colliderJson[Prefs::kColliderShape].get<std::string>(); // 定数を使用
                reactphysics3d::Transform localTransform = reactphysics3d::Transform::identity(); // TODO: Jsonから読み込む

                bool created = false;

                auto size = gameObject->GetScale();

                // BOX
                if (shapeType == Prefs::kColliderShapeBox && // 定数を使用
                    colliderJson.contains(Prefs::kColliderBoxHalfExtents) &&
                    colliderJson[Prefs::kColliderBoxHalfExtents].is_array() &&
                    colliderJson[Prefs::kColliderBoxHalfExtents].size() == 3)
                {
                    // 型チェックを追加
                    try
                    {
                        std::vector<float> he = colliderJson[Prefs::kColliderBoxHalfExtents].get<std::vector<float>>();
                        created = m_collider->CreateBox(GetRigidbody(), size, reactphysics3d::Vector3(he[0], he[1], he[2]),
                                                         localTransform);
                    }
                    catch (const nlohmann::json::type_error& e)
                    {
                        printf("Warning: Collider 'halfExtents' has incorrect type: %s\n", e.what());
                    }
                }
                // SPHERE
                else if (shapeType == Prefs::kColliderShapeSphere && // 定数を使用
                    colliderJson.contains(Prefs::kColliderSphereRadius) &&
                    colliderJson[Prefs::kColliderSphereRadius].is_number())
                {
                    try
                    {
                        float r = colliderJson[Prefs::kColliderSphereRadius].get<float>(); // 定数を使用
                        created = m_collider->CreateSphere(GetRigidbody(), size, r, localTransform);
                    }
                    catch (const nlohmann::json::type_error& e)
                    {
                        printf("Warning: Collider 'radius' (Sphere) has incorrect type: %s\n", e.what());
                    }
                }
                // CAPSULE
                else if (shapeType == Prefs::kColliderShapeCapsule && // 定数を使用
                    colliderJson.contains(Prefs::kColliderCapsuleRadius) && colliderJson[Prefs::kColliderCapsuleRadius].
                    is_number() &&
                    colliderJson.contains(Prefs::kColliderCapsuleHeight) && colliderJson[Prefs::kColliderCapsuleHeight].
                    is_number())
                {
                    try
                    {
                        float r = colliderJson[Prefs::kColliderCapsuleRadius].get<float>(); // 定数を使用
                        float h = colliderJson[Prefs::kColliderCapsuleHeight].get<float>(); // 定数を使用
                        created = m_collider->CreateCapsule(GetRigidbody(), size, r, h, localTransform);
                    }
                    catch (const nlohmann::json::type_error& e)
                    {
                        printf("Warning: Collider 'radius' or 'height' (Capsule) has incorrect type: %s\n", e.what());
                    }
                }

                if (!created)
                {
                    printf("Collider deserialization failed for shape: %s\n", shapeType.c_str());
                    m_collider.reset();
                }
                // Trigger設定の読み込み
                else if (colliderJson.contains(Prefs::kColliderIsTrigger) && // 定数を使用
                    colliderJson[Prefs::kColliderIsTrigger].is_boolean())
                {
                    try
                    {
                        bool isTrigger = colliderJson[Prefs::kColliderIsTrigger].get<bool>(); // 定数を使用
                        if (m_collider && m_collider->GetCollider()) // m_colliderのnullチェック追加
                        {
                            m_collider->GetCollider()->setIsTrigger(isTrigger);
                        }
                    }
                    catch (const nlohmann::json::type_error& e)
                    {
                        printf("Warning: Collider 'isTrigger' has incorrect type: %s\n", e.what());
                    }
                }
            }
            else
            {
                printf("Warning: Rigidbody 'collider' object is missing 'shape' string.\n");
            }
        }
    }

    std::string GetType() override
    {
        return "Rigidbody";
    }

    void OnGui() override
    {
        // RigidBodyが作成されていない場合は操作できない
        if (!m_RigidBody)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: RigidBody is not initialized!");
            return;
        }

        // --- Body Type の表示と変更 ---
        reactphysics3d::BodyType current_type = m_RigidBody->getType();
        const char* typeNames[] = {"STATIC", "KINEMATIC", "DYNAMIC"};
        int current_type_index = static_cast<int>(current_type);
        // ID衝突を避けるために "##..." を追加
        if (ImGui::Combo("Body Type##RBType", &current_type_index, typeNames, IM_ARRAYSIZE(typeNames)))
        {
            reactphysics3d::BodyType newType = static_cast<reactphysics3d::BodyType>(current_type_index);
            if (newType != current_type)
            {
                m_RigidBody->setType(newType);
            }
        }

        // --- 重力設定の表示と変更 ---
        bool is_gravity_enabled = m_RigidBody->isGravityEnabled();
        // ID衝突を避けるために "##..." を追加
        if (ImGui::Checkbox("Gravity Enabled##RBGravity", &is_gravity_enabled))
        {
            m_RigidBody->enableGravity(is_gravity_enabled);
        }

        ImGui::Separator();
        ImGui::Text("Collider"); // セクションヘッダー

        // --- Collider の表示、作成、削除 ---
        if (m_collider && m_collider->GetCollider()) // Colliderが存在する場合
        {
            ImGui::Text("  Shape: %s", EngineCollider::ShapeTypeToString(m_collider->GetShapeType()).c_str());

            // Collider固有のパラメータ編集UIを呼び出す
            auto goSize = gameObject->GetScale();
            m_collider->DrawShapeParamsGUI(goSize);

            ImGui::Spacing();
            if (ImGui::Button("Remove Collider##RBRemoveCollider"))
            {
                // 1. RigidBodyからColliderを削除
                //    (EngineColliderのデストラクタがShapeを破棄するので、先にColliderを破棄)
                if (m_collider->GetCollider())
                {
                    // 念のためチェック
                    // destroyCollider は内部で Shape への参照を解除する
                    m_RigidBody->removeCollider(m_collider->GetCollider());
                }
                // 2. EngineColliderインスタンスをリセット
                m_collider.reset(); // shared_ptrをnullptrにする (~EngineCollider()が呼ばれる)
            }
        }
        else // Colliderが存在しない場合
        {
            ImGui::Text("  Shape: None");

            // --- Collider作成UI ---
            const char* shapeOptions[] = {"Box", "Sphere", "Capsule"};
            static int selectedShapeIndex = 0; // GUI専用のstatic変数 (複数Rigidbody編集時は問題になる可能性あり)

            // ID衝突回避のため "##..."
            ImGui::Combo("Shape Type##AddColliderShape", &selectedShapeIndex, shapeOptions, IM_ARRAYSIZE(shapeOptions));

            // 各形状のパラメータ入力用変数 (static)
            static float boxHalfExtents[3] = {0.5f, 0.5f, 0.5f};
            static float sphereRadius = 0.5f;
            static float capsuleRadius = 0.5f;
            static float capsuleHeight = 1.0f;

            // 選択された形状に応じてパラメータ入力UIを表示
            switch (selectedShapeIndex)
            {
            case 0: // Box
                ImGui::InputFloat3("Half Extents##AddBox", boxHalfExtents);
                break;
            case 1: // Sphere
                ImGui::InputFloat("Radius##AddSphere", &sphereRadius, 0.01f, 0.1f, "%.2f");
                break;
            case 2: // Capsule
                ImGui::InputFloat("Radius##AddCapsuleR", &capsuleRadius, 0.01f, 0.1f, "%.2f");
                ImGui::InputFloat("Height##AddCapsuleH", &capsuleHeight, 0.01f, 0.1f, "%.2f");
                break;
            default: ;
            }

            // パラメータの最小値をクランプ (負の値やゼロを防ぐ)
            boxHalfExtents[0] = std::max(boxHalfExtents[0], 0.01f);
            boxHalfExtents[1] = std::max(boxHalfExtents[1], 0.01f);
            boxHalfExtents[2] = std::max(boxHalfExtents[2], 0.01f);
            sphereRadius = std::max(sphereRadius, 0.01f);
            capsuleRadius = std::max(capsuleRadius, 0.01f);
            capsuleHeight = std::max(capsuleHeight, 0.01f);


            if (ImGui::Button("Create Collider##RBCreateCollider"))
            {
                // m_colliderが既に存在しないか再確認 (ボタン連打対策)
                if (!m_collider)
                {
                    m_collider = std::make_shared<EngineCollider>();
                }

                bool success = false;
                const auto pos = gameObject->GetPosition();
                const auto quo = reactphysics3d::Quaternion::identity();
                const auto local_transform = reactphysics3d::Transform(reactphysics3d::Vector3(pos.x, pos.y, pos.z),
                                                                       quo);

                auto size = gameObject->GetScale();
                switch (selectedShapeIndex)
                {
                case 0: // Box
                    success = m_collider->CreateBox(GetRigidbody(), size,
                                                     reactphysics3d::Vector3(
                                                         boxHalfExtents[0] ,
                                                         boxHalfExtents[1] ,
                                                         boxHalfExtents[2] ),
                                                     local_transform);
                    break;
                case 1: // Sphere
                    success = m_collider->CreateSphere(GetRigidbody(), size, sphereRadius, local_transform);
                    break;
                case 2: // Capsule
                    success = m_collider->CreateCapsule(GetRigidbody(), size, capsuleRadius, capsuleHeight, local_transform);
                    break;
                default: ;
                }

                if (!success)
                {
                    printf("Failed to create collider!\n");
                    m_collider.reset(); // 失敗したらリセット
                }
            }
        }
    }

public:
    // 衝突イベント用のコールバック関数
    void OnCollisionEnter(GameObject* other)
    {
        m_CollidingObjects.insert(other);
    };

    void OnCollisionExit(GameObject* other)
    {
        m_CollidingObjects.erase(other);
    }

    // 状態を問い合わせる関数
    bool IsColliding() const { return !m_CollidingObjects.empty(); }
    const std::set<GameObject*>& GetCollidingObjects() const { return m_CollidingObjects; }

public:
    reactphysics3d::RigidBody* GetRigidbody() const { return m_RigidBody; }
    bool IsGravityEnabled() const { return m_RigidBody->isGravityEnabled(); }
    reactphysics3d::BodyType GetBodyType() const { return m_RigidBody->getType(); }
    std::shared_ptr<EngineCollider> GetColliderObject() const { return m_collider; }

private:
    reactphysics3d::RigidBody* m_RigidBody = nullptr;
    std::shared_ptr<EngineCollider> m_collider;
    std::set<GameObject*> m_CollidingObjects;
};
