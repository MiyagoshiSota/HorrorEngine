#pragma once
#include "Scene/Default/Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Physics/MyCollisionListener.h"
#include "Scene/ISceneBase.h"

class DefaultScene : public ISceneBase
{
public:
	DefaultScene() = default;
	~DefaultScene()
	{
		DefaultScene::shutdown();
	};

	void CreatePrimitiveObjects() override;
	bool Init(std::string go_file_path) override;
	void Update(float delta_time) override;
	void EditorUpdate(float delta_time) override;
	void Draw() override;
	void shutdown() override;

	void RebuidPhysicsWorld() override;
	void InitializeGameObject(std::string file_path) override;

public:
	std::shared_ptr<PostProcessManager> get_post_process_manager()
	{
		return m_default_pipeline_manager->get_post_process_manager();
	};

	
private:
	std::shared_ptr<DefaultPipelineManager> m_default_pipeline_manager;
	std::shared_ptr<MyCollisionListener> m_CollisionListener;
};

