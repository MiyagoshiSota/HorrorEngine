#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>
#include "Modules/ComPtr.h"

class GameObject;

/// Bottom Level Acceleration Structure (BLAS)
/// ジオメトリデータを格納する最下層の加速構造
class BottomLevelAS
{
public:
    BottomLevelAS() = default;
    ~BottomLevelAS() = default;

    /// ゲームオブジェクトからBLASを構築
    bool Build(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* commandList,
        const std::vector<std::shared_ptr<GameObject>>& gameObjects
    );

    /// 頂点・インデックスバッファから直接BLASを構築
    bool BuildFromBuffers(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* commandList,
        ID3D12Resource* vertexBuffer,
        UINT vertexCount,
        UINT vertexStride,
        ID3D12Resource* indexBuffer,
        UINT indexCount
    );

    ID3D12Resource* GetResult() const { return m_blasBuffer.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const 
    { 
        return m_blasBuffer ? m_blasBuffer->GetGPUVirtualAddress() : 0; 
    }

private:
    ComPtr<ID3D12Resource> m_blasBuffer;      // BLAS本体
    ComPtr<ID3D12Resource> m_scratchBuffer;   // 構築時の一時バッファ
};

/// Top Level Acceleration Structure (TLAS)
/// BLASのインスタンスを管理する最上層の加速構造
class TopLevelAS
{
public:
    TopLevelAS() = default;
    ~TopLevelAS() = default;

    /// インスタンス情報
    struct Instance
    {
        DirectX::XMMATRIX transform;      // ワールド変換行列
        UINT instanceID;                   // インスタンスID（シェーダーで使用）
        UINT instanceMask;                 // レイトレーシングマスク
        UINT contributionToHitGroupIndex; // ヒットグループテーブル先頭オフセット（RTGI用）
        D3D12_GPU_VIRTUAL_ADDRESS blasAddress; // 対応するBLASのアドレス
    };

    /// TLASを構築
    bool Build(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* commandList,
        const std::vector<Instance>& instances
    );

    ID3D12Resource* GetResult() const { return m_tlasBuffer.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const 
    { 
        return m_tlasBuffer ? m_tlasBuffer->GetGPUVirtualAddress() : 0; 
    }

private:
    ComPtr<ID3D12Resource> m_tlasBuffer;         // TLAS本体
    ComPtr<ID3D12Resource> m_scratchBuffer;      // 構築時の一時バッファ
    ComPtr<ID3D12Resource> m_instanceDescBuffer; // インスタンス記述バッファ
};

/// Acceleration Structure Manager
/// BLAS/TLASの構築と管理を行う
class AccelerationStructureManager
{
public:
    AccelerationStructureManager() = default;
    ~AccelerationStructureManager() = default;

    /// シーン全体のAcceleration Structureを構築
    bool BuildAccelerationStructures(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* commandList,
        const std::vector<std::shared_ptr<GameObject>>& gameObjects
    );

    /// TLASを更新（オブジェクトの移動に対応）
    bool UpdateTopLevelAS(
        ID3D12Device5* device,
        ID3D12GraphicsCommandList4* commandList,
        const std::vector<std::shared_ptr<GameObject>>& gameObjects
    );

    TopLevelAS* GetTopLevelAS() { return m_tlas.get(); }
    const std::vector<std::unique_ptr<BottomLevelAS>>& GetBottomLevelASList() const 
    { 
        return m_blasList; 
    }

    /// RTGI用: ジオメトリごとのVB/IB（BuildAccelerationStructuresで蓄積）
    struct GeometryBuffers { ID3D12Resource* vertexBuffer = nullptr; ID3D12Resource* indexBuffer = nullptr; };
    const std::vector<GeometryBuffers>& GetGeometryBuffers() const { return m_geometryBuffers; }
    const std::vector<UINT>& GetGeometryCountPerInstance() const { return m_geometryCountPerInstance; }
    UINT GetTotalGeometryCount() const { return static_cast<UINT>(m_geometryBuffers.size()); }

    /// ASが構築済みかどうかを確認
    bool IsBuilt() const { return m_tlas != nullptr && !m_blasList.empty(); }

private:
    std::unique_ptr<TopLevelAS> m_tlas;
    std::vector<std::unique_ptr<BottomLevelAS>> m_blasList;
    std::vector<GeometryBuffers> m_geometryBuffers;
    std::vector<UINT> m_geometryCountPerInstance;
};
