#pragma once
#include <reactphysics3d/collision/Collider.h>

#include "Core/App.h"

class EngineCollider
{
public:
	// 各種Getter
	reactphysics3d::Collider* get_collider() const { return m_Collider; }


	/// <summary>
	/// BoxShapeを使用したColliderを作成する
	/// </summary>
	/// <param name="rb"></param>
	/// <param name="transform"></param>
	/// <param name="direction"></param>
	void create_box_shape_collider(reactphysics3d::RigidBody* rb, const reactphysics3d::Transform& transform, const reactphysics3d::Vector3 direction)
	{
		reactphysics3d::BoxShape* shape = g_Scene->get_physics_common().createBoxShape(direction);
		add_collision(rb, shape, transform);
	}


	/// <summary>
	/// SphereShapeを使用したColliderを作成する
	/// </summary>
	/// <param name="rb"></param>
	/// <param name="transform"></param>
	/// <param name="radius"></param>
	void create_sphere_shape_collider(reactphysics3d::RigidBody* rb, const reactphysics3d::Transform& transform, const float radius)
	{
		reactphysics3d::SphereShape* shape = g_Scene->get_physics_common().createSphereShape(radius);
		add_collision(rb, shape, transform);
	}

	/// <summary>
	/// CapsuleShapeを使用したColliderを作成する
	/// </summary>
	/// <param name="rb"></param>
	/// <param name="transform"></param>
	/// <param name="height"></param>
	/// <param name="radius"></param>
	void create_capsule_shape_collider(reactphysics3d::RigidBody* rb, const reactphysics3d::Transform& transform, const float height, const float radius)
	{
		reactphysics3d::CapsuleShape* shape = g_Scene->get_physics_common().createCapsuleShape(height, height);
		add_collision(rb, shape, transform);
	}

	// TODO:MeshColliderの実装もしなきゃね.

private:

	/// <summary>
	/// Colliderを登録する
	/// 登録できるColliderは1つだけです、Simple is best
	/// </summary>
	/// <param name="rb"></param>
	/// <param name="shape"></param>
	/// <param name="transform"></param>
	/// <returns></returns>
	bool add_collision(reactphysics3d::RigidBody* rb, reactphysics3d::ConvexShape* shape, reactphysics3d::Transform transform)
	{
		m_Collider = rb->addCollider(shape, transform);

		if (m_Collider == nullptr) return false;
		return true;
	}

	// TODO:一旦1個のColliderのみ設定可能にしとく。Vectorとかで複数管理するべき。
	reactphysics3d::Collider* m_Collider = nullptr;
};
