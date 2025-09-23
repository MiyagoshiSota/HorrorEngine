#pragma once
#include <d3d12.h>
#include <Renderer/StandardShader/Struct/SharedStruct.h>
#include <Renderer/Graphics/Buffer/ConstantBuffer.h>
#include <Scene/GameObject/Model/Model.h>
#include <Scene/GameObject/Mesh/Mesh.h>
#include <Scene/GameObject/Material/Material.h>

class IGameObjectBase
{
public:
    IGameObjectBase(std::shared_ptr<Model> model) {
        m_Model = model;
        m_Transform = DirectX::XMMatrixIdentity();
    };

    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Draw(ID3D12GraphicsCommandList* cmdList) = 0;

    DirectX::XMMATRIX GetTransform() const { return m_Transform; }
    DirectX::XMFLOAT3 GetPosition() const { return m_Position; }
    std::shared_ptr<Model> GetModel() const { return m_Model; }
    std::shared_ptr<ConstantBuffer> GetConstantBuffer() const { return  constantBuffer; }

    void SetPosition(float x, float y, float z) { m_Position = { x, y, z }; }

    /// <summary>
    /// size分のコンスタントバッファを新しく作成する
    /// </summary>
    /// <param name="size"></param>
    void CreateConstantBuffer(size_t size) { constantBuffer = std::make_shared<ConstantBuffer>(size); }

protected:
    std::shared_ptr<Model> m_Model;

    std::shared_ptr<ConstantBuffer> constantBuffer = nullptr;

    DirectX::XMMATRIX m_Transform;
    DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f }; 
};