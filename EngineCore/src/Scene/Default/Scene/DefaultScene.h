#pragma once
#include "Scene/Default/Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Scene/Skybox/SkyboxManager.h"
#include "Scene/RayTracing/RayTracedShadowManager.h"
#include "Scene/RayTracing/RayTracedAOManager.h"
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

private:
	std::shared_ptr<DefaultPipelineManager> m_defaultPipelineManager;
	std::shared_ptr<MyCollisionListener> m_CollisionListener;
	std::unique_ptr<SkyboxManager> m_skyboxManager;
	std::unique_ptr<RayTracedShadowManager> m_rayTracedShadowManager;
	std::unique_ptr<RayTracedAOManager> m_rayTracedAOManager;
};

