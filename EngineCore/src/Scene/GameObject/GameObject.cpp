#include "GameObject.h"

void GameObject::init()
{

}

void GameObject::update()
{
    DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

    m_Transform = transMatrix;
}

void GameObject::draw(ID3D12GraphicsCommandList* cmdList)
{
}
