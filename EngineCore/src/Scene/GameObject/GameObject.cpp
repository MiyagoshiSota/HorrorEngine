#include "GameObject.h"

void GameObject::init()
{
	// コンポーネントのStart処理
	for (auto& comp : components)
	{
		comp->start();
	}

	// Scale, Rotation, PositionからTransform行列を計算

	// Scale
	DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);

	// Rotation
	DirectX::XMMATRIX rotXMat = DirectX::XMMatrixRotationRollPitchYaw(
		DirectX::XMConvertToRadians(m_Rotation.x),
		DirectX::XMConvertToRadians(m_Rotation.y),
		DirectX::XMConvertToRadians(m_Rotation.z)
	);

	// Position
	DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	m_Transform = scaleMat * rotXMat * transMatrix;
}

void GameObject::transform_update()
{
	// Scale
	DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);

	// Rotation
	DirectX::XMMATRIX rotXMat = DirectX::XMMatrixRotationRollPitchYaw(
		DirectX::XMConvertToRadians(m_Rotation.x),
		DirectX::XMConvertToRadians(m_Rotation.y),
		DirectX::XMConvertToRadians(m_Rotation.z)
	);

	// Position
	DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

	m_Transform = scaleMat * rotXMat * transMatrix;
}

void GameObject::component_update()
{
		// コンポーネントのUpdate処理
	for (auto& comp : components)
	{
		comp->update(0);
	}
}