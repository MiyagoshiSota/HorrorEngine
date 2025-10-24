#pragma once
#include <set>

#include "Core/App.h"
#include "Physics/EngineCollider.h"
#include "Scene/GameObject/Component/Component.h"

class Rigidbody : public Component
{
public:
	Rigidbody() = default;
	~Rigidbody() override {
		// このコンポーネントが破棄される時に、物理ボディも確実に破棄する
		if (m_RigidBody) {
			if (g_Scene && g_Scene->get_physics_world()) {
				g_Scene->get_physics_world()->destroyRigidBody(m_RigidBody);
			}
			m_RigidBody = nullptr;
		}
	}

	void start() override {}

	void update(float deltaTime) override {
		auto v3pos = m_RigidBody->getTransform().getPosition();

		m_GameObject->set_position(v3pos.x, v3pos.y, v3pos.z);
	}

	void deserialize(const nlohmann::json& jsonData, std::shared_ptr<GameObject> obj) override {
		// 親のGameObjectを保存
		m_GameObject = obj;

		// PhysicsWorldの取得
		auto world = g_Scene->get_physics_world();

		// PositionをReactPhysics3DのVector3型に変換
		auto fl_pos = obj->get_position();
		reactphysics3d::Vector3 pos = reactphysics3d::Vector3(fl_pos.x, fl_pos.y, fl_pos.z);

		// TODO:これもGameObjectから持ってくるべき
		// ReactPhysics3DのTransform型を作成
		reactphysics3d::Quaternion rot = reactphysics3d::Quaternion::identity();
		reactphysics3d::Transform transform = reactphysics3d::Transform(pos, rot);

		// RigidBodyの作成
		m_RigidBody = world->createRigidBody(transform);

		// 作成した物理ボディに、親であるGameObjectのポインタを保存する
		m_RigidBody->setUserData(obj.get());

		// HACK:以下rbに関する設定をjsonから読み込む処理だがハードコーディングされているから直す

		// 重力の設定
		if (jsonData.contains("isGravityEnabled"))
		{
			auto data = jsonData["isGravityEnabled"].get<bool>();
			m_RigidBody->enableGravity(data);
		}

		//　剛体のTypeをどうするか
		if (jsonData.contains("bodyType"))
		{
			auto body_type = jsonData["bodyType"].get<std::string>();
			if (body_type == "STATIC")
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

		// Colliderの生成
		if (jsonData.contains("isCollider"))
		{
			// EngineColliderの生成
			m_collider = std::make_shared<engine_collider>();

			const auto is_collider_nlohmann = jsonData["isCollider"];

			// Colliderの生成に失敗したらエラーを出す
			if (!m_collider->create_collider(is_collider_nlohmann, m_RigidBody))
			{
				printf("Colliderの生成に失敗");
			}
		}
	}

	std::string get_type() override {
		return "Rigidbody";
	}

	void on_gui() override {
		ImGui::Text("Rigidbody Component");
		ImGui::Separator();

		// Body Typeの表示
		auto body_type = m_RigidBody->getType();
		std::string body_type_str;
		switch (body_type)
		{
		case reactphysics3d::BodyType::STATIC:
			body_type_str = "STATIC";
			break;
		case reactphysics3d::BodyType::DYNAMIC:
			body_type_str = "DYNAMIC";
			break;
		case reactphysics3d::BodyType::KINEMATIC:
			body_type_str = "KINEMATIC";
			break;
		default:
			body_type_str = "UNKNOWN";
			break;
		}
		ImGui::Text("Body Type: %s", body_type_str.c_str());

		// 重力の表示
		bool is_gravity_enabled = m_RigidBody->isGravityEnabled();
		ImGui::Text("Gravity Enabled: %s", is_gravity_enabled ? "True" : "False");
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
	reactphysics3d::RigidBody* get_rigidbody() const { return m_RigidBody; }
	std::shared_ptr<engine_collider> get_collider() const { return m_collider; }
	bool is_gravity_enabled() const { return m_RigidBody->isGravityEnabled(); }
	reactphysics3d::BodyType get_body_type() const { return m_RigidBody->getType(); }
	std::shared_ptr<engine_collider> get_collider_object() const { return m_collider; }
	
private:
	std::shared_ptr<GameObject> m_GameObject;
	reactphysics3d::RigidBody* m_RigidBody = nullptr;
	std::shared_ptr<engine_collider> m_collider;
	std::set<GameObject*> m_CollidingObjects;
};
