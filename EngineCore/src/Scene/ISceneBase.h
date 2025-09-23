#pragma once
#include <d3d12.h>
#include "Scene/Camera/SceneCamera.h"
#include "Scene/Renderer/SceneRenderer.h"
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

protected:
	std::unique_ptr<SceneCamera> m_Camera;
	std::unique_ptr<SceneRenderer> m_Renderer;
	std::unique_ptr<SceneResourceManager> m_SceneResourceManager;
	std::vector<std::shared_ptr <IGameObjectBase>> m_GameObjects;
};