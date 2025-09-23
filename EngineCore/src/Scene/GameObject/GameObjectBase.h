#pragma once
#include <d3d12.h>
#include <Renderer/StandardShader/Struct/SharedStruct.h>
#include <Renderer/Graphics/Buffer/ConstantBuffer.h>
#include <Scene/GameObject/Model/Model.h>
#include <Scene/GameObject/Mesh/Mesh.h>
#include <Scene/GameObject/Material/Material.h>

class GameObjectBase
{
public:
    GameObjectBase(std::shared_ptr<Model> model);
    void Init();
    void Update(float speed);
    void Draw(ID3D12GraphicsCommandList* cmdList);
    DirectX::XMMATRIX GetTransform() const { return m_Transform; }

    // 位置を設定するメソッドを追加
    void SetPosition(float x, float y, float z) { m_Position = { x, y, z }; }

    std::shared_ptr<Model> m_Model;
    ConstantBuffer* constantBuffer = nullptr;

private:
    DirectX::XMMATRIX m_Transform;
    DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f }; // 位置
    float m_RotateY = 0.0f;                             // 回転角度
};