#pragma once
#include <memory>
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Modules/ComPtr.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Renderer/RayTracing/RayTracingPipelineState.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"
#include "RayTracedGITarget.h"
#include "Renderer/RayTracing/AccelerationStructure.h"

struct ID3D12Device5;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
class GameObject;

/// RTGI用定数バッファ（b0）
struct RayTracedGIConstants
{
    DirectX::XMFLOAT3 cameraPosition;
    float radius;
    float bias;
    float indirectIntensity;
    float padding0;
    UINT frameIndex;
    UINT numRaysPerPixel;
    DirectX::XMFLOAT3 skyColor;
    float padding1;
    UINT vertexStrideBytes; // 頂点ストライド（SharedStruct::Vertex = 60）
    UINT padding2[3];
};

/// RTGI Passに渡す描画用データ
struct RayTracedGIRenderData
{
    AccelerationStructureManager* asManager = nullptr;
    RayTracingPipelineState* pipelineState = nullptr;
    ShaderBindingTable* shaderBindingTable = nullptr;
    ID3D12Resource* giOutputResource = nullptr;
    ID3D12DescriptorHeap* descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE tlasSrvCpuHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE tlasSrvGpuHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE giUavGpuHandle = { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE clearUavCpuHandle = { 0 };
    UINT descriptorIncrementSize = 0;
    ID3D12Resource* constantBuffer = nullptr;
    D3D12_RESOURCE_STATES* pGiOutputState = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool isValid = false;
    std::shared_ptr<RayTracedGITarget> giTarget;
    class RayTracedGIManager* giManager = nullptr; // EnsureGeometryDescriptorsAndSBT 用
};

/// RTGIのリソース管理（RTPSO/SBT/UAV/ディスクリプタ/定数バッファ）
class RayTracedGIManager
{
public:
    static constexpr UINT kFrameBufferCount = 2;

    RayTracedGIManager() = default;
    ~RayTracedGIManager() = default;

    bool Init(ID3D12Device5* device, UINT width, UINT height);

    RayTracedGIRenderData GetRenderData(UINT frameIndex, AccelerationStructureManager* asManager) const;

    void UpdateConstants(const RayTracedGIConstants& constants, UINT frameIndex);
    void SetGiOutputState(D3D12_RESOURCE_STATES state);
    bool IsValid() const { return m_initialized; }

    /// ジオメトリバッファのByteAddress SRV・アルベドテクスチャSRVとSBTを更新（AS構築後に呼ぶ）
    void EnsureGeometryDescriptorsAndSBT(
        ID3D12Device5* device,
        const AccelerationStructureManager* asManager,
        const std::vector<std::shared_ptr<GameObject>>& gameObjects);

private:
    bool CreateClearUavHeap(ID3D12Device5* device);
    void CreateByteAddressSRV(ID3D12Device5* device, ID3D12Resource* buffer, UINT sizeInBytes, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle);

    bool m_initialized = false;
    UINT m_width = 0;
    UINT m_height = 0;

    std::unique_ptr<RayTracingPipelineState> m_pipelineState;
    std::unique_ptr<ShaderBindingTable> m_shaderBindingTable;

    ComPtr<ID3D12Resource> m_giOutputResource;
    std::shared_ptr<DescriptorHandle> m_rtgiDescriptors;
    ComPtr<ID3D12DescriptorHeap> m_clearUavHeap;
    ComPtr<ID3D12Resource> m_constantBuffers[kFrameBufferCount];

    std::shared_ptr<RayTracedGITarget> m_giTarget;
    D3D12_RESOURCE_STATES m_giOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    UINT m_cachedGeometryCount = 0;
    std::shared_ptr<DescriptorHandle> m_geometryVBIbDescriptors;
};
