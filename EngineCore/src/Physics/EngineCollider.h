#pragma once
#include <reactphysics3d/collision/Collider.h>

#include "Core/App.h"

class engine_collider
{
private:
	/// <summary>
	/// 返すColliderの形状とTransformをまとめた構造体
	/// </summary>
	struct collider_game_object
	{
		reactphysics3d::ConvexShape* shape;
		reactphysics3d::Transform transform;

	public:
		collider_game_object(reactphysics3d::ConvexShape* s, reactphysics3d::Transform t) : shape(s), transform(t) {}
		reactphysics3d::ConvexShape* get_shape() const { return shape; }
		reactphysics3d::Transform get_transform() const { return transform; }
	};

	/// <summary>
	/// CapsuleShapeのパラメータをまとめた構造体
	/// </summary>
	struct capsule_object
	{
		float height;
		float radius;

	public:
		capsule_object(float h, float r) : height(h), radius(r) {}
		float get_height() const { return height; }
		float get_radius() const { return radius; }
	};

public:
	bool create_collider(const nlohmann::json& jsonData,reactphysics3d::RigidBody* rb)
	{
		// ShapeTypeの取得(未設定だったらemptyのオブジェクトを返す)
		const auto shape_type = jsonData["shape"].get<std::string>();
		if (shape_type == "") return false;

		// Transformはとりあえずidentityで初期化
		auto transform = reactphysics3d::Transform::identity();

		// Colliderの形状に応じて作成処理を分岐
		reactphysics3d::ConvexShape* shape = nullptr;
		if (shape_type == "Box") {
			auto data = deserialize_box_shape_collider(jsonData);
			shape = create_box_shape_collider(rb,transform,data);
		}
		else if (shape_type == "Sphere")
		{
			auto data = deserialize_sphere_shape_collider(jsonData);
			shape = create_sphere_shape_collider(rb, transform, data);
		}
		else if (shape_type == "Capsule")
		{
			auto data = deserialize_capsule_shape_collider(jsonData);
			shape = create_capsule_shape_collider(rb, transform, data.get_height(), data.get_radius());
		}
		else {
			// 未知のShapeTypeが指定された場合のエラーハンドリング
			std::cerr << "Error: Unknown shapeType '" << shape_type << "' in EngineCollider deserialization." << std::endl;

			return false;
		}

		// ColliderObjectsにColliderの形状とTransformを保存
		m_ColliderObjects = std::make_shared<collider_game_object>(shape, transform);

		return true;
	}

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

	/// <summary>
	/// BoxShapeを使用したColliderを作成する
	/// </summary>
	/// <param name="rb"></param>
	/// <param name="transform"></param>
	/// <param name="direction"></param>
	reactphysics3d::ConvexShape* create_box_shape_collider(reactphysics3d::RigidBody* rb, const reactphysics3d::Transform& transform, const reactphysics3d::Vector3 direction)
	{
		reactphysics3d::BoxShape* shape = g_Scene->get_physics_common().createBoxShape(direction);
		add_collision(rb, shape, transform);
		return shape;
	}


	/// <summary>
	/// SphereShapeを使用したColliderを作成する
	/// </summary>
	/// <param name="rb"></param>
	/// <param name="transform"></param>
	/// <param name="radius"></param>
	reactphysics3d::ConvexShape* create_sphere_shape_collider(reactphysics3d::RigidBody* rb, const reactphysics3d::Transform& transform, const float radius)
	{
		reactphysics3d::SphereShape* shape = g_Scene->get_physics_common().createSphereShape(radius);
		add_collision(rb, shape, transform);
		return shape;
	}

	/// <summary>
	/// CapsuleShapeを使用したColliderを作成する
	/// </summary>
	/// <param name="rb"></param>
	/// <param name="transform"></param>
	/// <param name="height"></param>
	/// <param name="radius"></param>
	reactphysics3d::ConvexShape* create_capsule_shape_collider(reactphysics3d::RigidBody* rb, const reactphysics3d::Transform& transform, const float height, const float radius)
	{
		reactphysics3d::CapsuleShape* shape = g_Scene->get_physics_common().createCapsuleShape(height, radius);
		add_collision(rb, shape, transform);
		return shape;
	}


	/// <summary>
	/// BoxShapeのパラメータをjsonから読み込む
	/// </summary>
	/// <param name="jsonData"></param>
	/// <returns></returns>
	reactphysics3d::Vector3 deserialize_box_shape_collider(const nlohmann::json& jsonData)
	{
		const auto shape_type_nlohmann = jsonData["direction"].get<std::vector<float>>();

		// Directionをreactphysics3d::Vector3にキャスト
		reactphysics3d::Vector3 direction;
		if (shape_type_nlohmann.size() == 3) {
			direction = reactphysics3d::Vector3(shape_type_nlohmann[0], shape_type_nlohmann[1], shape_type_nlohmann[2]);
		}
		else {
			direction = reactphysics3d::Vector3(0.5f, 0.5f, 0.5f);
		}
		return direction;
	}

	/// <summary>
	/// SphereShapeのパラメータをjsonから読み込む
	/// </summary>
	/// <param name="jsonData"></param>
	/// <returns></returns>
	float deserialize_sphere_shape_collider(const nlohmann::json& jsonData)
	{
		const auto radius = jsonData["radius"].get<float>();
		return radius;
	}

	/// <summary>
	/// CapsuleShapeのパラメータをjsonから読み込む
	/// </summary>
	/// <param name="jsonData"></param>
	/// <returns></returns>
	engine_collider::capsule_object deserialize_capsule_shape_collider(const nlohmann::json& jsonData)
	{
		const auto height = jsonData["height"].get<float>();
		const auto radius = jsonData["radius"].get<float>();

		capsule_object capsule(height, radius);
		return capsule;
	}

	// TODO:MeshColliderの実装もしなきゃね.

	// TODO:一旦1個のColliderのみ設定可能にしとく。Vectorとかで複数管理するべき。
	reactphysics3d::Collider* m_Collider = nullptr;

public:
	std::shared_ptr<collider_game_object> m_ColliderObjects = nullptr;
};
