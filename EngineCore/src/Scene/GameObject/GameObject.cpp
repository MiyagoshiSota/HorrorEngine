#include "GameObject.h"

void GameObject::init()
{
	// コンポーネントのStart処理
	for (auto& comp : components)
	{
		comp->start();
	}
}

void GameObject::update()
{
    DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

    m_Transform = transMatrix;

	// コンポーネントのUpdate処理
	for (auto& comp : components)
	{
		comp->update(0);
	}
}

void GameObject::draw(ID3D12GraphicsCommandList* cmdList)
{
}
