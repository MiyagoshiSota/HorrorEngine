#pragma once
#include "Scene/GameObject/IGameObjectBase.h"

class DefaultGameObject : public IGameObjectBase
{
public:
	DefaultGameObject(std::shared_ptr<Model> model) : IGameObjectBase(model) {

	}

	void Init() override;
	void Update() override;
	void Draw(ID3D12GraphicsCommandList* cmdList) override;

private:
	float m_RotateY;
};

