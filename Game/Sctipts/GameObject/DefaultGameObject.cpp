#include "DefaultGameObject.h"

void DefaultGameObject::Init()
{
    m_RotateY = 0;
}

void DefaultGameObject::Update()
{
    m_RotateY += 0.05;

    DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixRotationY(m_RotateY);
    DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

    m_Transform = rotMatrix * transMatrix;
}

void DefaultGameObject::Draw(ID3D12GraphicsCommandList* cmdList)
{
}
