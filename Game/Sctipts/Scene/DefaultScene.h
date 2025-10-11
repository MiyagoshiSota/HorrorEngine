#pragma once
#include "Scene/ISceneBase.h"

class DefaultScene : public ISceneBase
{
public:
	DefaultScene() = default;
	~DefaultScene()
	{
		DefaultScene::shutdown();
	};

	bool Init() override;
	void Update(float delta_time) override;
	void Draw() override;
	void shutdown() override;

	void RebuidPhysicsWorld() override;
	void InitializeGameObject();

public:
	void EditorUpdate();
};

