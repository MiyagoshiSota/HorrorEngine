#include "GameObject.h"

void GameObject::init()
{
	// コンポーネントのStart処理
	for (auto& comp : components)
	{
		comp->start();
	}

	// 初期位置に基づいて変換行列を計算
	DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	m_Transform = transMatrix;
}

void GameObject::transform_update()
{
    DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

    m_Transform = transMatrix;
}

void GameObject::component_update()
{
		// コンポーネントのUpdate処理
	for (auto& comp : components)
	{
		comp->update(0);
	}
}