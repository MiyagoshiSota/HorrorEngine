#include "GameObjectBase.h" // 対応するヘッダをインクルード

// コンストラクタの実装例
GameObjectBase::GameObjectBase(std::shared_ptr<Model> model)
    : m_Model(model)
{
    m_Transform = DirectX::XMMatrixIdentity(); // 初期化
}

void GameObjectBase::Init()
{
}



void GameObjectBase::Update(float speed)
{
    // 1. 回転角度を更新
    m_RotateY += speed;

    // 2. 回転と移動の行列をそれぞれ計算
    DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixRotationY(m_RotateY);
    DirectX::XMMATRIX transMatrix = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

    // 3. 回転行列と移動行列を合成し、m_Transformに結果を保存する (★★★ これが最も重要 ★★★)
    m_Transform = rotMatrix * transMatrix;
}