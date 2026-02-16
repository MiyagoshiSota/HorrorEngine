#pragma once

#include "Audio/AudioManager.h"
#include "Camera/SceneCamera.h"
#include "Renderer/PipelineManager/PipelineStateManager.h"
#include "ResourceManager/SceneResourceManager.h"
#include "Time/TimeManager.h"
#include <reactphysics3d/reactphysics3d.h>

#include "Renderer/Light/LightingManager.h"
#include "Renderer/PipelineManager/IPipelineManager.h"
#include <algorithm>

class ISceneBase
{
public:
	ISceneBase() {
	}

	virtual ~ISceneBase() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	/// <returns></returns>
	virtual bool Init(std::string goFilePath) = 0;

	/// <summary>
	/// 終了処理
	/// </summary>
	virtual void Shutdown() = 0;

	/// <summary>
	/// 毎フレーム実行
	/// </summary>
	virtual void Update(float deltaTime) {
		FlushPendingGameObjectChanges();
		m_isInUpdateLoop = true;
		for (auto& obj : m_GameObjects)
		{
			obj->TransformUpdate();
			obj->ComponentUpdate(deltaTime);
		}
		m_isInUpdateLoop = false;
		// TimeManagerの更新
		m_TimeManager->Update();
	};

	/// <summary>
	/// Editorモード時の毎フレーム実行
	/// </summary>
	virtual void EditorUpdate(float deltaTime) {
		FlushPendingGameObjectChanges();
		m_isInUpdateLoop = true;
		for (auto& obj : m_GameObjects)
		{
			obj->TransformUpdate();
		}
		m_isInUpdateLoop = false;
		// TimeManagerの更新
		m_TimeManager->Update();
	};

	/// <summary>
	/// 描画系
	/// </summary>
	virtual void Draw() = 0;

	/// <summary>
	/// PhysicsWorldの再構築
	/// </summary>
	virtual void RebuidPhysicsWorld() = 0;

	/// <summary>
	/// ゲームオブジェクトの初期化
	/// </summary>
	virtual void InitializeGameObject(std::string goFilePath) = 0;

	// --Getter--
	// Manager
	std::shared_ptr<SceneCamera> GetSceneCamera() { return m_Camera; }
	std::shared_ptr<PipelineStateManager> GetPipelineStateManager() { return m_PipelineStateManager; }
	std::shared_ptr<IPipelineManager> GetPipelineManager() { return m_PipelineManager; }
	std::shared_ptr<TimeManager> GetTimeManager() { return m_TimeManager; }
	std::shared_ptr<AudioManager> GetAudioManager() { return m_AudioManager; }
	std::shared_ptr<LightingManager> GetLightingManager() { return m_LightingManager; }

	// Object
	std::vector<std::shared_ptr <GameObject>> GetGameObjects() { return m_GameObjects; }
	
	/// <summary>
	/// ゲームオブジェクトの追加
	/// </summary>
	void AddGameObject(std::shared_ptr<GameObject> go)
	{
		if (m_isInUpdateLoop)
			m_pendingGameObjectsToAdd.push_back(std::move(go));
		else
			m_GameObjects.push_back(std::move(go));
	}
	
	/// <summary>
	/// ゲームオブジェクトの削除
	/// <summary>
	void RemoveGameObject(std::shared_ptr<GameObject> go)
	{
		if (m_isInUpdateLoop)
		{
			m_pendingGameObjectsToRemove.push_back(go);
			return;
		}
		const auto it = std::find_if(m_GameObjects.begin(), m_GameObjects.end(),
			[&go](const std::shared_ptr<GameObject>& p) { return p == go; });
		if (it != m_GameObjects.end())
			m_GameObjects.erase(it);
	}

	//	Physics
	reactphysics3d::PhysicsWorld* GetPhysicsWorld() const { return m_physicsWorld; }
	reactphysics3d::PhysicsCommon& GetPhysicsCommon() { return physics_common; }

	// --Setter--

	/// <summary>
	/// ゲームオブジェクトの設定
	/// </summary>
	void SetAllGameObject(std::vector<std::shared_ptr <GameObject>> game_objects) { m_GameObjects = game_objects; }


	// --de/serializer--
	virtual bool SerializeGameObjects(const std::string& goFilePath) = 0;
	virtual void DeserializeGameObjects(const std::string& goFilePath) = 0;
	
protected:
	// Manager
	std::shared_ptr<IPipelineManager> m_PipelineManager;
	std::shared_ptr<PipelineStateManager> m_PipelineStateManager;
	std::shared_ptr<AudioManager> m_AudioManager;
	std::shared_ptr<TimeManager> m_TimeManager;
	std::shared_ptr<LightingManager> m_LightingManager;

	// Object
	std::shared_ptr<SceneCamera> m_Camera;
	std::vector<std::shared_ptr<GameObject>> m_GameObjects;
	std::vector<std::shared_ptr<GameObject>> m_pendingGameObjectsToAdd;
	std::vector<std::shared_ptr<GameObject>> m_pendingGameObjectsToRemove;
	bool m_isInUpdateLoop = false;

	/// <summary>
	/// 待機中のゲームオブジェクトの処理
	/// </summary>
	void FlushPendingGameObjectChanges()
	{
		for (auto& go : m_pendingGameObjectsToRemove)
		{
			const auto it = std::find_if(m_GameObjects.begin(), m_GameObjects.end(),
				[&go](const std::shared_ptr<GameObject>& p) { return p == go; });
			if (it != m_GameObjects.end())
				m_GameObjects.erase(it);
		}
		m_pendingGameObjectsToRemove.clear();
		for (auto& go : m_pendingGameObjectsToAdd)
			m_GameObjects.push_back(std::move(go));
		m_pendingGameObjectsToAdd.clear();
	}

	// Physics
	reactphysics3d::PhysicsCommon physics_common;
	reactphysics3d::PhysicsWorld* m_physicsWorld;
};
