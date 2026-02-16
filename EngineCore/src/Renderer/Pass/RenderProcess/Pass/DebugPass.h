#pragma once
#include "Physics/Component/Rigidbody.h"
#include "Renderer/Pass/RenderProcess/SceneRenderPassBase.h"

class DebugPass : public SceneRenderPassBase
{
public:
    struct DebugConstants {
        DirectX::XMMATRIX world;
    };

    struct DebugInfo
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    static const int kFrameBufferCount = 2;
    
    static const UINT kMaxDebugTriangleVertices = 65536;
    
    DebugPass(UINT maxObjectsPerFrame = 1024)
    {
        m_debugConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(DebugConstants));

        // DebugRenderer用の頂点バッファをフレーム数分作成する
        m_debugTriangleVertexBuffers.resize(kFrameBufferCount);
    
        const UINT stride = sizeof(DebugInfo);
        const UINT vertexBufferSize = stride * kMaxDebugTriangleVertices;

        for (int i = 0; i < kFrameBufferCount; ++i)
        {
            m_debugTriangleVertexBuffers[i] = std::make_shared<VertexBuffer>(vertexBufferSize, stride, nullptr);
        
            if (!m_debugTriangleVertexBuffers[i] || !m_debugTriangleVertexBuffers[i]->IsValid())
            {
                printf("DebugPass: Failed to create vertex buffer %d\n", i);
            }
        }
    };
    ~DebugPass() override = default;

protected:
    void Collect(RenderContext& context) override;
    void Draw(RenderContext& context) override;

private:
    std::string m_psoName = "DebugWireframePSO";
    std::string m_rootSignatureName = "DebugRootSignature";

    std::vector<Rigidbody*> m_collidersToDraw;

    std::shared_ptr<Mesh> m_unitCubeMesh;
    std::shared_ptr<Mesh> m_unitSphereMesh;

    // リングバッファ用のメンバー
    std::shared_ptr<ConstantBuffer> m_debugConstantBufferRing; // リングバッファ全体
    UINT m_maxObjectsPerFrame;
    UINT m_currentFrameIndex; // フレームインデックス (0 or 1 for double buffering)
    UINT m_currentObjectIndex; // 現在のフレームで何番目のオブジェクトか
    DebugConstants* m_mappedConstantBufferPtr = nullptr; // マップされたCPUポインタの先頭
    D3D12_GPU_VIRTUAL_ADDRESS m_currentFrameStartGpuAddress = 0; // 現在フレームのGPUアドレス先頭

    std::shared_ptr<VertexBuffer> m_debugVertexBuffer = nullptr;
    std::vector<DebugConstants> m_debugVertices;

    std::vector<std::shared_ptr<VertexBuffer>> m_debugTriangleVertexBuffers;

    std::shared_ptr<ConstantBuffer> m_debugConstantBuffer;

    std::vector<DebugInfo> m_debugTriangleVertices;
    
    DirectX::XMMATRIX ToDXMatrix(const reactphysics3d::Transform& transform);
};
