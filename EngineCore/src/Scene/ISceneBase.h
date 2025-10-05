#pragma once
#include <d3d12.h>

#include "Renderer/PipelineManager/IPipelineManager.h"
#include "Scene/Camera/SceneCamera.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include "Scene/GameObject/IGameObjectBase.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"

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
	virtual void Update() {
		// GameObjectのUpdate処理
		for (auto& obj : m_GameObjects)
		{
			obj->Update();
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
	std::vector<std::shared_ptr <IGameObjectBase>> get_game_objects() { return m_GameObjects; }

protected:
	std::unique_ptr<IPipelineManager> m_PipelineManager;
	std::shared_ptr<SceneCamera> m_Camera;
	std::shared_ptr<PipelineStateManager> m_PipelineStateManager;
	std::unique_ptr<SceneResourceManager> m_SceneResourceManager;
	std::vector<std::shared_ptr <IGameObjectBase>> m_GameObjects;
};