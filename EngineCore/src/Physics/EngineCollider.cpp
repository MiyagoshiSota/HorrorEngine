#include "EngineCollider.h"

#include "imgui.h"
#include "Core/App.h"

engine_collider::~engine_collider()
{
    // デストラクタで CollisionShape を PhysicsCommon から削除
    if (m_shape_ && g_Scene)
    {
        reactphysics3d::PhysicsCommon& physics_common = g_Scene->get_physics_common();
        switch (m_shape_type_)
        {
        case ShapeType::BOX:
            physics_common.destroyBoxShape(
                dynamic_cast<reactphysics3d::BoxShape*>(m_shape_));
            break;
        case ShapeType::SPHERE:
            physics_common.destroySphereShape(
                dynamic_cast<reactphysics3d::SphereShape*>(m_shape_));
            break;
        case ShapeType::CAPSULE:
            physics_common.destroyCapsuleShape(
                dynamic_cast<reactphysics3d::CapsuleShape*>(m_shape_));
            break;
        default:
            break; // UNKNOWN or other types
        }
        m_shape_ = nullptr; // ポインタをクリア
    }
}

bool engine_collider::add_collider_internal(
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
    if (m_shape_)
    {
        // 古い形状はデストラクタに任せる（ここで破棄すると新しい形状も破棄されうる）
        // PhysicsCommon::destroy...Shape を呼ぶのは ~engine_collider() で行う
        m_shape_ = nullptr;
        m_shape_type_ = ShapeType::UNKNOWN;
    }

    m_shape_ = shape; // 新しい形状を保持
    m_collider_ = rb->addCollider(shape, transform);
    return (m_collider_ != nullptr);
}

bool engine_collider::create_box(reactphysics3d::RigidBody* rb,
                                 const reactphysics3d::Vector3& halfExtents,
                                 const reactphysics3d::Transform& transform)
{
    if (!g_Scene)
        return false;
    reactphysics3d::BoxShape* shape =
        g_Scene->get_physics_common().createBoxShape(halfExtents);
    if (add_collider_internal(rb, shape, transform))
    {
        m_shape_type_ = ShapeType::BOX;
        m_box_half_extents_ = halfExtents; // パラメータを保存
        return true;
    }
    // 失敗した場合: add_collider_internal が古い形状をクリアしているので、
    // 新しく作った形状だけ破棄する
    g_Scene->get_physics_common().destroyBoxShape(shape);
    return false;
}

bool engine_collider::create_sphere(reactphysics3d::RigidBody* rb, float radius,
                                    const reactphysics3d::Transform& transform)
{
    if (!g_Scene)
        return false;
    reactphysics3d::SphereShape* shape =
        g_Scene->get_physics_common().createSphereShape(radius);
    if (add_collider_internal(rb, shape, transform))
    {
        m_shape_type_ = ShapeType::SPHERE;
        m_sphere_radius_ = radius; // パラメータを保存
        return true;
    }
    g_Scene->get_physics_common().destroySphereShape(shape);
    return false;
}

bool engine_collider::create_capsule(reactphysics3d::RigidBody* rb, float radius,
                                     float height,
                                     const reactphysics3d::Transform& transform)
{
    if (!g_Scene)
        return false;
    reactphysics3d::CapsuleShape* shape =
        g_Scene->get_physics_common().createCapsuleShape(radius, height);
    if (add_collider_internal(rb, shape, transform))
    {
        m_shape_type_ = ShapeType::CAPSULE;
        m_capsule_radius_ = radius; // パラメータを保存
        m_capsule_height_ = height;
        return true;
    }
    g_Scene->get_physics_common().destroyCapsuleShape(shape);
    return false;
}

// --- Parameter Getters ---
reactphysics3d::Vector3 engine_collider::get_box_half_extents() const
{
    if (m_shape_type_ == ShapeType::BOX && m_shape_)
    {
        // 実行時の形状から取得 (ただし、RP3Dではconstメソッドがない場合がある)
        // return static_cast<reactphysics3d::BoxShape*>(m_Shape)->getHalfExtents();
        // 代わりに保存した値を返す
        return m_box_half_extents_;
    }
    return m_box_half_extents_; // 保存値またはデフォルト値
}

float engine_collider::get_sphere_radius() const
{
    if (m_shape_type_ == ShapeType::SPHERE && m_shape_)
    {
        // return static_cast<reactphysics3d::SphereShape*>(m_Shape)->getRadius();
        return m_sphere_radius_;
    }
    return m_sphere_radius_;
}

float engine_collider::get_capsule_radius() const
{
    if (m_shape_type_ == ShapeType::CAPSULE && m_shape_)
    {
        // return static_cast<reactphysics3d::CapsuleShape*>(m_Shape)->getRadius();
        return m_capsule_radius_;
    }
    return m_capsule_radius_;
}

float engine_collider::get_capsule_height() const
{
    if (m_shape_type_ == ShapeType::CAPSULE && m_shape_)
    {
        // return static_cast<reactphysics3d::CapsuleShape*>(m_Shape)->getHeight();
        return m_capsule_height_;
    }
    return m_capsule_height_;
}

// --- Shape Parameter Editing UI ---
void engine_collider::DrawShapeParamsGUI()
{
    if (!m_collider_ || !m_shape_)
        return;

    bool paramsChanged = false;
    ImGui::Indent(); // パラメータを見やすくインデント

    switch (m_shape_type_)
    {
    case ShapeType::BOX:
        {
            // InputFloat3は現在の値を変更するため、一時変数を使わない
            if (ImGui::InputFloat3("Half Extents", &m_box_half_extents_.x))
            {
                paramsChanged = true;
            }
            break;
        }
    case ShapeType::SPHERE:
        {
            if (ImGui::InputFloat("Radius", &m_sphere_radius_))
            {
                paramsChanged = true;
            }
            break;
        }
    case ShapeType::CAPSULE:
        {
            if (ImGui::InputFloat("Radius", &m_capsule_radius_))
                paramsChanged = true;
            if (ImGui::InputFloat("Height", &m_capsule_height_))
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
            // 現在のRigidbodyを取得
            reactphysics3d::RigidBody* rb =
                dynamic_cast<reactphysics3d::RigidBody*>(m_collider_->getBody());
            if (rb)
            {
                // 現在のローカルトランスフォームを取得
                reactphysics3d::Transform currentLocalTransform =
                    m_collider_->getLocalToBodyTransform();
                // 形状タイプに基づいて再作成
                switch (m_shape_type_)
                {
                case ShapeType::BOX:
                    create_box(rb, m_box_half_extents_, currentLocalTransform);
                    break;
                case ShapeType::SPHERE:
                    create_sphere(rb, m_sphere_radius_, currentLocalTransform);
                    break;
                case ShapeType::CAPSULE:
                    create_capsule(rb, m_capsule_radius_, m_capsule_height_,
                                   currentLocalTransform);
                    break;
                default:
                    break; // 何もしない
                }
            }
        }
    }

    // Trigger設定
    bool is_trigger = m_collider_->getIsTrigger();
    if (ImGui::Checkbox("Is Trigger", &is_trigger))
    {
        m_collider_->setIsTrigger(is_trigger);
    }

    ImGui::Unindent(); // インデント解除

    // TODO: Material (friction, bounciness) の設定UIも追加可能
}

// --- Helper Function ---
// static なのでクラス名のスコープ解決が必要
std::string engine_collider::shape_type_to_string(ShapeType type)
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
