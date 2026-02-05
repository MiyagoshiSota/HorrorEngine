#pragma once
#include <memory>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Modules/ComPtr.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Renderer/RayTracing/RayTracingPipelineState.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"
#include "RayTracedAOTarget.h"

struct ID3D12Device5;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;

/// RTAO用定数バッファ（b0）
struct RayTracedAOConstants
{
    DirectX::XMFLOAT3 cameraPosition;
    float radius;
    float bias;
    float padding0;
    float padding1;
    UINT frameIndex;
    UINT numRaysPerPixel;
};

/// RTAO Passに渡す描画用データ（エンジンヒープをそのまま使用し、CopyDescriptors を避ける）
struct RayTracedAORenderData
{
    AccelerationStructureManager* asManager = nullptr;
    RayTracingPipelineState* pipelineState = nullptr;
    ShaderBindingTable* shaderBindingTable = nullptr;
    ID3D12Resource* aoOutputResource = nullptr;
    ID3D12DescriptorHeap* descriptorHeap = nullptr;       /// エンジンのシェーダー可視ヒープ
    D3D12_CPU_DESCRIPTOR_HANDLE tlasSrvCpuHandle = { 0 }; /// 毎フレーム TLAS SRV を書き込むスロット
    D3D12_GPU_DESCRIPTOR_HANDLE tlasSrvGpuHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE aoUavGpuHandle = { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE clearUavCpuHandle = { 0 };
    UINT descriptorIncrementSize = 0;
    ID3D12Resource* constantBuffer = nullptr;
    D3D12_RESOURCE_STATES* pAoOutputState = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool isValid = false;
    std::shared_ptr<RayTracedAOTarget> aoTarget;
};

/// RTAOのリソース管理（RTPSO/SBT/UAV/ディスクリプタヒープ/定数バッファ）
class RayTracedAOManager
{
public:
    static constexpr UINT kFrameBufferCount = 2;

    RayTracedAOManager() = default;
    ~RayTracedAOManager() = default;

    bool Init(ID3D12Device5* device, UINT width, UINT height);

    /// asManager はシーン側（RayTracedShadowManager）のものを渡す
    RayTracedAORenderData GetRenderData(UINT frameIndex, AccelerationStructureManager* asManager) const;

    void UpdateConstants(const RayTracedAOConstants& constants, UINT frameIndex);
    void SetAoOutputState(D3D12_RESOURCE_STATES state);
    bool IsValid() const { return m_initialized; }

private:
    bool CreateClearUavHeap(ID3D12Device5* device);

    bool m_initialized = false;
    UINT m_width = 0;
    UINT m_height = 0;

    std::unique_ptr<RayTracingPipelineState> m_pipelineState;
    std::unique_ptr<ShaderBindingTable> m_shaderBindingTable;

    ComPtr<ID3D12Resource> m_aoOutputResource;
    std::shared_ptr<DescriptorHandle> m_rtaoDescriptors; /// エンジンヒープから Allocate(2): TLAS SRV, AO UAV
    ComPtr<ID3D12DescriptorHeap> m_clearUavHeap;
    ComPtr<ID3D12Resource> m_constantBuffers[kFrameBufferCount];

    std::shared_ptr<RayTracedAOTarget> m_aoTarget;
    D3D12_RESOURCE_STATES m_aoOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
};
