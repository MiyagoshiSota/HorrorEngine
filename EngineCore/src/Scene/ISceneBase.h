#pragma once

#include "Audio/AudioManager.h"
#include "Camera/SceneCamera.h"
#include "Renderer/PipelineManager/PipelineStateManager.h"
#include "ResourceManager/SceneResourceManager.h"
#include "Time/TimeManager.h"
#include <reactphysics3d/reactphysics3d.h>

#include "Renderer/Light/LightingManager.h"
#include "Renderer/PipelineManager/IPipelineManager.h"

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
	virtual bool Init(std::string go_file_path) = 0;

	/// <summary>
	/// 終了処理
	/// </summary>
	virtual void shutdown() = 0;

	/// <summary>
	/// 毎フレーム実行
	/// </summary>
	virtual void Update(float delta_time) {
		// GameObjectのUpdate処理
		for (auto& obj : m_GameObjects)
		{
			obj->transform_update();
			obj->component_update();
		}
		
		// TimeManagerの更新
		m_TimeManager->update();
	};

	/// <summary>
	/// Editorモード時の毎フレーム実行
	/// </summary>
	virtual void EditorUpdate(float delta_time) {
		// GameObjectのUpdate処理
		for (auto& obj : m_GameObjects)
		{
			obj->transform_update();
		}

		// TimeManagerの更新
		m_TimeManager->update();
	};

	/// <summary>
	/// 描画系
	/// </summary>
	virtual void Draw() = 0;

	virtual void RebuidPhysicsWorld() = 0;

	virtual void InitializeGameObject(std::string go_file_path) = 0;

	// --Getter--

	// Manager
	std::shared_ptr<SceneCamera> get_scene_camera() { return m_Camera; }
	std::shared_ptr<PipelineStateManager> get_pipeline_state_manager() { return m_PipelineStateManager; }
	std::shared_ptr<IPipelineManager> get_pipeline_manager() { return m_PipelineManager; }
	std::unique_ptr<SceneResourceManager> get_scene_resource_manager() { return std::move(m_SceneResourceManager); }
	std::shared_ptr<TimeManager> get_time_manager() { return m_TimeManager; }
	std::shared_ptr<AudioManager> get_audio_manager() { return m_AudioManager; }
	std::shared_ptr<LightingManager> get_lighting_manager() { return m_LightingManager; }

	// Object
	std::vector<std::shared_ptr <GameObject>> get_game_objects() { return m_GameObjects; }
	void add_game_object(std::shared_ptr<GameObject> go) { m_GameObjects.push_back(go); }

	//	Physics
	reactphysics3d::PhysicsWorld* get_physics_world() const { return m_physicsWorld; }
	reactphysics3d::PhysicsCommon& get_physics_common() { return physics_common; }

	// --Setter--
	void set_all_game_object(std::vector<std::shared_ptr <GameObject>> game_objects) { m_GameObjects = game_objects; }


	// --de/serializer--
	virtual bool serialize_game_objects(const std::string& go_file_path) = 0;
	virtual void deserialize_game_objects(const std::string& go_file_path) = 0;
	
protected:
	// Manager
	std::shared_ptr<IPipelineManager> m_PipelineManager;
	std::shared_ptr<PipelineStateManager> m_PipelineStateManager;
	std::unique_ptr<SceneResourceManager> m_SceneResourceManager;
	std::shared_ptr<AudioManager> m_AudioManager;
	std::shared_ptr<TimeManager> m_TimeManager;
	std::shared_ptr<LightingManager> m_LightingManager;

	// Object
	// TODO:CameraはGameObjectにしようかな...
	std::shared_ptr<SceneCamera> m_Camera;
	std::vector<std::shared_ptr<GameObject>> m_GameObjects;

	// Physics
	reactphysics3d::PhysicsCommon physics_common;
	reactphysics3d::PhysicsWorld* m_physicsWorld;
};
