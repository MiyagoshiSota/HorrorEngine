#pragma once
#include "Scene/ISceneBase.h"

class DefaultScene : public ISceneBase
{
public:
	DefaultScene() = default;

	bool Init() override;
	void Update() override;
	void Draw() override;
};

