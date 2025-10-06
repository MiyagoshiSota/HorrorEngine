#pragma once
#include <d3d12.h>

#include "Renderer/PipelineManager/IPipelineManager.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include "Scene/GameObject/GameObject.h"
#include <reactphysics3d/reactphysics3d.h>

class ISceneBase
{
public:
	ISceneBase() {
		m_SceneResourceManager = std::make_unique<SceneResourceManager>();
	}

	virtual ~ISceneBase() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <returns></returns>
	virtual bool Init() = 0;

	/// <summary>
	/// 毎フレーム実行
	/// </summary>
	virtual void Update(float delta_time) {
		// GameObjectのUpdate処理
		for (auto& obj : m_GameObjects)
		{
			obj->update();
		}
	};

	/// <summary>
	/// 描画系
	/// </summary>
	virtual void Draw() = 0;

	// Getter
	std::shared_ptr<SceneCamera> get_scene_camera() { return m_Camera; }
	std::shared_ptr<PipelineStateManager> get_pipeline_state_manager() { return m_PipelineStateManager; }
	std::unique_ptr<SceneResourceManager> get_scene_resource_manager() { return std::move(m_SceneResourceManager); }
	std::vector<std::shared_ptr <GameObject>> get_game_objects() { return m_GameObjects; }
	std::map<std::string, std::shared_ptr<Model>> get_models() { return m_models; }
	reactphysics3d::PhysicsWorld* get_physics_world() const { return m_physicsWorld; }
	std::shared_ptr<Model> get_model(const std::string& path)
	{
		if (m_models.find(path) != m_models.end()) {
			return m_models[path];
		}
		return nullptr;
	}

protected:
	// Manager
	std::unique_ptr<IPipelineManager> m_PipelineManager;
	std::shared_ptr<PipelineStateManager> m_PipelineStateManager;
	std::unique_ptr<SceneResourceManager> m_SceneResourceManager;

	// Object
	// TODO:CameraはGameObjectにしようかな...
	std::shared_ptr<SceneCamera> m_Camera;
	std::vector<std::shared_ptr <GameObject>> m_GameObjects;

	// Model
	std::map<std::string ,std::shared_ptr<Model>> m_models;

	// Physics
	reactphysics3d::PhysicsCommon physics_common;
	reactphysics3d::PhysicsWorld* m_physicsWorld;
};