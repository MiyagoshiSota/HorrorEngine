#pragma once
#include "Core/App.h"
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

	void start() override {
	}

	void update(float deltaTime) override {
		auto v3pos = m_RigidBody->getTransform().getPosition();

		m_GameObject->set_position(v3pos.x, v3pos.y, v3pos.z);
	}

	void deserialize(const nlohmann::json& jsonData, std::shared_ptr<GameObject> obj) override {
		m_GameObject = obj;

		auto world = g_Scene->get_physics_world();

		auto fl_pos = obj->get_position();
		reactphysics3d::Vector3 pos = reactphysics3d::Vector3(fl_pos.x, fl_pos.y, fl_pos.z);

		// TODO:これもGameObjectから持ってくるべき
		reactphysics3d::Quaternion rot = reactphysics3d::Quaternion::identity();
		reactphysics3d::Transform transform = reactphysics3d::Transform(pos, rot);

		m_RigidBody = world->createRigidBody(transform);

		if (jsonData.contains("isGravityEnabled"))
		{
			m_RigidBody->enableGravity(true);
		}

		m_RigidBody->setType(reactphysics3d::BodyType::DYNAMIC);

		reactphysics3d::Vector3 force(2.0, 0.0, 0.0);

		// Apply a force to the center of the body
		m_RigidBody->applyWorldForceAtCenterOfMass(force);
	}

private:
	std::shared_ptr<GameObject> m_GameObject;
	reactphysics3d::RigidBody* m_RigidBody = nullptr;
};
