#include "EngineCollider.h"

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE
#include "Component/Rigidbody.h"
#include "Core/App.h"

EngineCollider::~EngineCollider()
{
    // デストラクタで CollisionShape を PhysicsCommon から削除
    if (m_shape_ && g_Scene)
    {
        reactphysics3d::PhysicsCommon& physicsCommon = g_Scene->GetPhysicsCommon();
        switch (m_shapeType)
        {
        case ShapeType::BOX:
            physicsCommon.destroyBoxShape(
                dynamic_cast<reactphysics3d::BoxShape*>(m_shape_));
            break;
        case ShapeType::SPHERE:
            physicsCommon.destroySphereShape(
                dynamic_cast<reactphysics3d::SphereShape*>(m_shape_));
            break;
        case ShapeType::CAPSULE:
            physicsCommon.destroyCapsuleShape(
                dynamic_cast<reactphysics3d::CapsuleShape*>(m_shape_));
            break;
        default:
            break; // UNKNOWN or other types
        }
        m_shape_ = nullptr; // ポインタをクリア
    }
}

bool EngineCollider::AddColliderInternal(
    reactphysics3d::RigidBody* rb, reactphysics3d::CollisionShape* shape,
    const reactphysics3d::Transform& transform)
{
    if (!rb || !shape)
        return false;

    // 既存のColliderとShapeがあれば破棄 (複数持たない設計)
    if (m_collider_)
    {
        rb->removeCollider(m_collider_);
        m_collider_ = nullptr;
    }
    if (m_shape_ && g_Scene)
    {
        // 既存の形状を PhysicsCommon で破棄してから差し替える（リーク防止）
        reactphysics3d::PhysicsCommon& physicsCommon = g_Scene->GetPhysicsCommon();
        switch (m_shapeType)
        {
        case ShapeType::BOX:
            physicsCommon.destroyBoxShape(
                dynamic_cast<reactphysics3d::BoxShape*>(m_shape_));
            break;
        case ShapeType::SPHERE:
            physicsCommon.destroySphereShape(
                dynamic_cast<reactphysics3d::SphereShape*>(m_shape_));
            break;
        case ShapeType::CAPSULE:
            physicsCommon.destroyCapsuleShape(
                dynamic_cast<reactphysics3d::CapsuleShape*>(m_shape_));
            break;
        default:
            break;
        }
        m_shape_ = nullptr;
        m_shapeType = ShapeType::UNKNOWN;
    }

    m_shape_ = shape; // 新しい形状を保持
    m_collider_ = rb->addCollider(shape, transform);
    return (m_collider_ != nullptr);
}

bool EngineCollider::CreateBox(reactphysics3d::RigidBody* rb,
DirectX::XMFLOAT3 goSize,
                                 const reactphysics3d::Vector3& halfExtents,
                                 const reactphysics3d::Transform& transform)
{
    if (!g_Scene)
        return false;
    auto size_v = reactphysics3d::Vector3(goSize.x, goSize.y, goSize.z);
    // 物理形状はローカル半 extents × Scale で作成（見た目と一致）。保存はローカルのみで二重スケールを防ぐ
    const reactphysics3d::Vector3 scaledHalfExtents(
        halfExtents.x * size_v.x,
        halfExtents.y * size_v.y,
        halfExtents.z * size_v.z);

    reactphysics3d::BoxShape* shape =
        g_Scene->GetPhysicsCommon().createBoxShape(scaledHalfExtents);
    if (AddColliderInternal(rb, shape, transform))
    {
        m_shapeType = ShapeType::BOX;
        m_boxHalfExtents = halfExtents; // ローカル値のみ保存（Apply/再読込で巨大化しない）
        m_localPosition = transform.getPosition();
        return true;
    }
    g_Scene->GetPhysicsCommon().destroyBoxShape(shape);
    return false;
}

bool EngineCollider::CreateSphere(reactphysics3d::RigidBody* rb,DirectX::XMFLOAT3 goSize, float radius,
                                    const reactphysics3d::Transform& transform)
{
    if (!g_Scene)
        return false;
    auto size_v = reactphysics3d::Vector3(goSize.x, goSize.y, goSize.z);
    const reactphysics3d::decimal avgScale = (size_v.x + size_v.y + size_v.z) / static_cast<reactphysics3d::decimal>(3);
    const reactphysics3d::decimal scaledRadius = radius * avgScale;

    reactphysics3d::SphereShape* shape =
        g_Scene->GetPhysicsCommon().createSphereShape(scaledRadius);
    if (AddColliderInternal(rb, shape, transform))
    {
        m_shapeType = ShapeType::SPHERE;
        m_sphereRadius = radius; // ローカル値のみ保存
        m_localPosition = transform.getPosition();
        return true;
    }
    g_Scene->GetPhysicsCommon().destroySphereShape(shape);
    return false;
}

bool EngineCollider::CreateCapsule(reactphysics3d::RigidBody* rb, DirectX::XMFLOAT3 goSize,float radius,
                                     float height,
                                     const reactphysics3d::Transform& transform)
{
    if (!g_Scene)
        return false;
    auto size_v = reactphysics3d::Vector3(goSize.x, goSize.y, goSize.z);
    const reactphysics3d::decimal avgScale = (size_v.x + size_v.y + size_v.z) / static_cast<reactphysics3d::decimal>(3);
    const reactphysics3d::decimal scaledRadius = radius * avgScale;
    const reactphysics3d::decimal scaledHeight = height * avgScale;

    reactphysics3d::CapsuleShape* shape =
        g_Scene->GetPhysicsCommon().createCapsuleShape(scaledRadius, scaledHeight);
    if (AddColliderInternal(rb, shape, transform))
    {
        m_shapeType = ShapeType::CAPSULE;
        m_capsuleRadius = radius;   // ローカル値のみ保存
        m_capsuleHeight = height;
        m_localPosition = transform.getPosition();
        return true;
    }
    g_Scene->GetPhysicsCommon().destroyCapsuleShape(shape);
    return false;
}

// --- Parameter Getters ---
reactphysics3d::Vector3 EngineCollider::GetBoxHalfExtents() const
{
    if (m_shapeType == ShapeType::BOX && m_shape_)
    {
        // 実行時の形状から取得 (ただし、RP3Dではconstメソッドがない場合がある)
        // return static_cast<reactphysics3d::BoxShape*>(m_Shape)->getHalfExtents();
        // 代わりに保存した値を返す
        return m_boxHalfExtents;
    }
    return m_boxHalfExtents; // 保存値またはデフォルト値
}

float EngineCollider::GetSphereRadius() const
{
    if (m_shapeType == ShapeType::SPHERE && m_shape_)
    {
        // return static_cast<reactphysics3d::SphereShape*>(m_Shape)->getRadius();
        return m_sphereRadius;
    }
    return m_sphereRadius;
}

float EngineCollider::GetCapsuleRadius() const
{
    if (m_shapeType == ShapeType::CAPSULE && m_shape_)
    {
        // return static_cast<reactphysics3d::CapsuleShape*>(m_Shape)->getRadius();
        return m_capsuleRadius;
    }
    return m_capsuleRadius;
}

float EngineCollider::GetCapsuleHeight() const
{
    if (m_shapeType == ShapeType::CAPSULE && m_shape_)
    {
        // return static_cast<reactphysics3d::CapsuleShape*>(m_Shape)->getHeight();
        return m_capsuleHeight;
    }
    return m_capsuleHeight;
}

// --- Shape Parameter Editing UI ---
#ifndef BUILD_STANDALONE
void EngineCollider::DrawShapeParamsGUI(DirectX::XMFLOAT3 goSize)
{
    if (!m_collider_ || !m_shape_)
        return;

    bool paramsChanged = false;
    ImGui::Indent(); // パラメータを見やすくインデント

    // オフセット（剛体中心からのローカル位置）
    if (ImGui::InputFloat3("Offset", &m_localPosition.x))
        paramsChanged = true;

    switch (m_shapeType)
    {
    case ShapeType::BOX:
        {
            // InputFloat3は現在の値を変更するため、一時変数を使わない
            if (ImGui::InputFloat3("Half Extents", &m_boxHalfExtents.x))
            {
                paramsChanged = true;
            }
            break;
        }
    case ShapeType::SPHERE:
        {
            if (ImGui::InputFloat("Radius", &m_sphereRadius))
            {
                paramsChanged = true;
            }
            break;
        }
    case ShapeType::CAPSULE:
        {
            if (ImGui::InputFloat("Radius", &m_capsuleRadius))
                paramsChanged = true;
            if (ImGui::InputFloat("Height", &m_capsuleHeight))
                paramsChanged = true;
            break;
        }
    default:
        ImGui::Text("Shape parameters not editable.");
        break;
    }

    // TODO: ReactPhysics3Dの形状は動的にサイズ変更できないため、
    //       パラメータが変更されたら形状を再作成する必要がある。
    if (paramsChanged)
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                           "Parameters changed. Apply Needed.");
        // Applyボタンなどを表示して再作成を促す
        if (ImGui::Button("Apply Changes"))
        {
            reactphysics3d::RigidBody* rb =
                dynamic_cast<reactphysics3d::RigidBody*>(m_collider_->getBody());
            if (rb)
            {
                const reactphysics3d::Transform localTransform(
                    m_localPosition,
                    reactphysics3d::Quaternion::identity());
                switch (m_shapeType)
                {
                case ShapeType::BOX:
                    CreateBox(rb, goSize, m_boxHalfExtents, localTransform);
                    break;
                case ShapeType::SPHERE:
                    CreateSphere(rb, goSize, m_sphereRadius, localTransform);
                    break;
                case ShapeType::CAPSULE:
                    CreateCapsule(rb, goSize, m_capsuleRadius, m_capsuleHeight, localTransform);
                    break;
                default:
                    break; // 何もしない
                }
            }
        }
    }

    // Trigger設定
    bool isTrigger = m_collider_->getIsTrigger();
    if (ImGui::Checkbox("Is Trigger", &isTrigger))
    {
        m_collider_->setIsTrigger(isTrigger);
    }

    ImGui::Unindent(); // インデント解除

    // TODO: Material (friction, bounciness) の設定UIも追加可能
}
#else
void EngineCollider::DrawShapeParamsGUI(DirectX::XMFLOAT3 goSize)
{
    // BUILD_STANDALONE時は何もしない
}
#endif // BUILD_STANDALONE

// --- Helper Function ---
// static なのでクラス名のスコープ解決が必要
std::string EngineCollider::ShapeTypeToString(ShapeType type)
{
    switch (type)
    {
    case ShapeType::BOX:
        return "Box";
    case ShapeType::SPHERE:
        return "Sphere";
    case ShapeType::CAPSULE:
        return "Capsule";
    default:
        return "Unknown";
    }
}
