#pragma once
#include "Scene/ISceneBase.h"

class DefaultScene : public ISceneBase
{
public:
	DefaultScene() = default;

	bool Init() override;
	void Update(float delta_time) override;
	void Draw() override;
};

