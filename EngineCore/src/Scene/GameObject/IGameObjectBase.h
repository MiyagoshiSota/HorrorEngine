#pragma once
#include <d3d12.h>
#include <Renderer/StandardShader/Struct/SharedStruct.h>
#include <Renderer/Graphics/Buffer/ConstantBuffer.h>
#include <Scene/GameObject/Model/Model.h>
#include <Scene/GameObject/Mesh/Mesh.h>

#include "Renderer/Engine.h"

class IGameObjectBase
{
public:
    IGameObjectBase(std::shared_ptr<Model> model) {
        m_Model = model;
        m_Transform = DirectX::XMMatrixIdentity();

        // コンストラクタでフレーム数分のバッファを全て作成する
        m_ConstantBuffers.resize(g_Engine->FRAME_BUFFER_COUNT);
        for (int i = 0; i < g_Engine->FRAME_BUFFER_COUNT; ++i)
        {
            m_ConstantBuffers[i] = std::make_shared<ConstantBuffer>(sizeof(SharedStruct::Transform));
        }
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

    // フレームインデックスを受け取るように変更
    std::shared_ptr<ConstantBuffer> GetConstantBuffer(UINT frameIndex) const {
        return m_ConstantBuffers[frameIndex];
    }

protected:
    std::shared_ptr<Model> m_Model;

    std::shared_ptr<ConstantBuffer> constantBuffer = nullptr;
    std::vector<std::shared_ptr<ConstantBuffer>> m_ConstantBuffers;

    DirectX::XMMATRIX m_Transform;
    DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f }; 
};
