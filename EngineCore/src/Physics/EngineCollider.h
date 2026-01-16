#pragma once

#include <reactphysics3d/reactphysics3d.h>
#include <string>
#include <DirectXMath.h>

class engine_collider // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
    // --- Enums ---
    // 対応する形状タイプ
    enum class ShapeType { BOX, SPHERE, CAPSULE, UNKNOWN };

    // --- Constructor / Destructor ---
    engine_collider() = default;
    ~engine_collider(); // デストラクタで形状を破棄

    // --- Collider Creation Methods ---
    bool create_box(reactphysics3d::RigidBody* rb, DirectX::XMFLOAT3 go_size,
                    const reactphysics3d::Vector3& halfExtents,
                    const reactphysics3d::Transform& transform =
                        reactphysics3d::Transform::identity());
    bool create_sphere(reactphysics3d::RigidBody* rb, DirectX::XMFLOAT3 go_size, float radius,
                       const reactphysics3d::Transform& transform =
                           reactphysics3d::Transform::identity());
    bool create_capsule(reactphysics3d::RigidBody* rb, DirectX::XMFLOAT3 go_size, float radius, float height,
                        const reactphysics3d::Transform& transform =
                            reactphysics3d::Transform::identity());

    // --- Getters ---
    reactphysics3d::Collider* get_collider() const { return m_collider_; }
    ShapeType get_shape_type() const { return m_shape_type_; }
    reactphysics3d::Vector3 get_box_half_extents() const;
    float get_sphere_radius() const;
    float get_capsule_radius() const;
    float get_capsule_height() const;

    // --- GUI Drawing ---
    void DrawShapeParamsGUI(DirectX::XMFLOAT3 go_size); // シェイプ固有のパラメータ編集UI

    // --- Helpers ---
    // ShapeTypeを文字列に変換するヘルパー
    static std::string shape_type_to_string(ShapeType type);

private:
    // --- Private Methods ---
    bool add_collider_internal(reactphysics3d::RigidBody* rb,
                               reactphysics3d::CollisionShape* shape,
                               const reactphysics3d::Transform& transform);

    // --- Member Variables ---
    reactphysics3d::Collider* m_collider_ = nullptr;
    reactphysics3d::CollisionShape* m_shape_ = nullptr; // 形状へのポインタも保持
    ShapeType m_shape_type_ = ShapeType::UNKNOWN;

    // パラメータ保持用（GUIでの再編集やシリアライズ用）
    reactphysics3d::Vector3 m_box_half_extents_ = {0.5f, 0.5f, 0.5f};
    float m_sphere_radius_ = 0.5f;
    float m_capsule_radius_ = 0.5f;
    float m_capsule_height_ = 1.0f;
};
