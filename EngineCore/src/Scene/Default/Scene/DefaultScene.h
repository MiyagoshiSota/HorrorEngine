#pragma once
#include "Scene/Default/Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Scene/Skybox/SkyboxManager.h"
#include "Scene/RayTracing/RayTracedShadowManager.h"
#include "Scene/RayTracing/RayTracedAOManager.h"
#include "Scene/RayTracing/RayTracedGIManager.h"
#include "Scene/RayTracing/RayTracedReflectionManager.h"
#include "Physics/MyCollisionListener.h"
#include "Scene/ISceneBase.h"

class DefaultScene : public ISceneBase
{
public:
	DefaultScene() = default;
	~DefaultScene()
	{
		DefaultScene::Shutdown();
	};
	
	bool Init(std::string goFilePath) override;
	void Update(float deltaTime) override;
	void EditorUpdate(float deltaTime) override;
	void Draw() override;
	void Shutdown() override;

	void RebuidPhysicsWorld() override;
	void InitializeGameObject(std::string filePath) override;

	void ApplyPlayModeCamera(float deltaTime);

	bool SerializeGameObjects(const std::string& goFilePath) override;
	void DeserializeGameObjects(const std::string& goFilePath) override;

public:
	std::shared_ptr<PostProcessManager> GetPostProcessManager()
	{
		return m_defaultPipelineManager->GetPostProcessManager();
	};

	std::shared_ptr<DefaultPipelineManager> GetDefaultPipelineManager() { return m_defaultPipelineManager; }

	SkyboxManager* GetSkyboxManager() { return m_skyboxManager.get(); }
	RayTracedShadowManager* GetRayTracedShadowManager() { return m_rayTracedShadowManager.get(); }
	RayTracedAOManager* GetRayTracedAOManager() { return m_rayTracedAOManager.get(); }
	RayTracedGIManager* GetRayTracedGIManager() { return m_rayTracedGIManager.get(); }
	RayTracedReflectionManager* GetRayTracedReflectionManager() { return m_rayTracedReflectionManager.get(); }

private:
	std::shared_ptr<DefaultPipelineManager> m_defaultPipelineManager;
	std::shared_ptr<MyCollisionListener> m_CollisionListener;
	std::unique_ptr<SkyboxManager> m_skyboxManager;
	std::unique_ptr<RayTracedShadowManager> m_rayTracedShadowManager;
	std::unique_ptr<RayTracedAOManager> m_rayTracedAOManager;
	std::unique_ptr<RayTracedGIManager> m_rayTracedGIManager;
	std::unique_ptr<RayTracedReflectionManager> m_rayTracedReflectionManager;
};

