#pragma once

#include <reactphysics3d/reactphysics3d.h>
#include <string>
#include <DirectXMath.h>

class EngineCollider // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
    // --- Enums ---
    // 対応する形状タイプ
    enum class ShapeType { BOX, SPHERE, CAPSULE, UNKNOWN };

    // --- Constructor / Destructor ---
    EngineCollider() = default;
    ~EngineCollider(); // デストラクタで形状を破棄

    // --- Collider Creation Methods ---
    bool CreateBox(reactphysics3d::RigidBody* rb, DirectX::XMFLOAT3 goSize,
                    const reactphysics3d::Vector3& halfExtents,
                    const reactphysics3d::Transform& transform =
                        reactphysics3d::Transform::identity());
    bool CreateSphere(reactphysics3d::RigidBody* rb, DirectX::XMFLOAT3 goSize, float radius,
                       const reactphysics3d::Transform& transform =
                           reactphysics3d::Transform::identity());
    bool CreateCapsule(reactphysics3d::RigidBody* rb, DirectX::XMFLOAT3 goSize, float radius, float height,
                        const reactphysics3d::Transform& transform =
                            reactphysics3d::Transform::identity());

    // --- Getters ---
    reactphysics3d::Collider* GetCollider() const { return m_collider_; }
    ShapeType GetShapeType() const { return m_shapeType; }
    reactphysics3d::Vector3 GetLocalPosition() const { return m_localPosition; }
    reactphysics3d::Vector3 GetBoxHalfExtents() const;
    float GetSphereRadius() const;
    float GetCapsuleRadius() const;
    float GetCapsuleHeight() const;

    // --- GUI Drawing ---
    void DrawShapeParamsGUI(DirectX::XMFLOAT3 goSize); // シェイプ固有のパラメータ編集UI

    // --- Helpers ---
    // ShapeTypeを文字列に変換するヘルパー
    static std::string ShapeTypeToString(ShapeType type);

private:
    // --- Private Methods ---
    bool AddColliderInternal(reactphysics3d::RigidBody* rb,
                               reactphysics3d::CollisionShape* shape,
                               const reactphysics3d::Transform& transform);

    // --- Member Variables ---
    reactphysics3d::Collider* m_collider_ = nullptr;
    reactphysics3d::CollisionShape* m_shape_ = nullptr; // 形状へのポインタも保持
    ShapeType m_shapeType = ShapeType::UNKNOWN;

    // パラメータ保持用（GUIでの再編集やシリアライズ用）
    reactphysics3d::Vector3 m_localPosition = {0.0f, 0.0f, 0.0f}; // 剛体からのローカルオフセット
    reactphysics3d::Vector3 m_boxHalfExtents = {0.5f, 0.5f, 0.5f};
    float m_sphereRadius = 0.5f;
    float m_capsuleRadius = 0.5f;
    float m_capsuleHeight = 1.0f;
};
