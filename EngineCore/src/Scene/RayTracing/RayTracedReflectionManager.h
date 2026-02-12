#pragma once
#include <memory>
#include <vector>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Modules/ComPtr.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Renderer/RayTracing/RayTracingPipelineState.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"
#include "RayTracedReflectionTarget.h"
#include "Renderer/RayTracing/AccelerationStructure.h"

struct ID3D12Device5;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
class GameObject;

/// RT Reflection 用定数バッファ（b0）
/// 注意: HLSL cbuffer では配列が16バイト境界にアラインされるため、個別の変数を使用
struct RayTracedReflectionConstants
{
    DirectX::XMFLOAT3 cameraPosition;
    float bias;
    float maxDistance;
    float reflectionIntensity;
    float roughnessThreshold;
    float fresnelF0;
    UINT frameIndex;
    UINT padding1_0;
    UINT padding1_1;
    UINT padding1_2;
    DirectX::XMFLOAT3 skyColor;
    float padding2;
    UINT vertexStrideBytes;
    UINT padding3_0;
    UINT padding3_1;
    UINT padding3_2;
};

/// RT Reflection Pass に渡す描画用データ
struct RayTracedReflectionRenderData
{
    AccelerationStructureManager* asManager = nullptr;
    RayTracingPipelineState* pipelineState = nullptr;
    ShaderBindingTable* shaderBindingTable = nullptr;
    ID3D12Resource* reflectionOutputResource = nullptr;
    ID3D12DescriptorHeap* descriptorHeap = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE tlasSrvCpuHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE tlasSrvGpuHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE reflectionUavGpuHandle = { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE clearUavCpuHandle = { 0 };
    UINT descriptorIncrementSize = 0;
    ID3D12Resource* constantBuffer = nullptr;
    D3D12_RESOURCE_STATES* pReflectionOutputState = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool isValid = false;
    std::shared_ptr<RayTracedReflectionTarget> reflectionTarget;
    class RayTracedReflectionManager* reflectionManager = nullptr;
};

/// RT Reflection のリソース管理（RTPSO / SBT / UAV / 定数バッファ）
class RayTracedReflectionManager
{
public:
    static constexpr UINT kFrameBufferCount = 2;

    RayTracedReflectionManager() = default;
    ~RayTracedReflectionManager() = default;

    bool Init(ID3D12Device5* device, UINT width, UINT height);

    RayTracedReflectionRenderData GetRenderData(UINT frameIndex, AccelerationStructureManager* asManager) const;

    void UpdateConstants(const RayTracedReflectionConstants& constants, UINT frameIndex);
    void SetReflectionOutputState(D3D12_RESOURCE_STATES state);
    bool IsValid() const { return m_initialized; }

    void EnsureGeometryDescriptorsAndSBT(
        ID3D12Device5* device,
        const AccelerationStructureManager* asManager,
        const std::vector<std::shared_ptr<GameObject>>& gameObjects);

    /// 診断用: true のときジオメトリごとに別色の 1x1 テクスチャをアルベドにバインドする（SBT のヒットグループ選択を確認）
    void SetDebugGeometryColors(bool enable) { m_debugGeometryColors = enable; }
    bool IsDebugGeometryColors() const { return m_debugGeometryColors; }

private:
    bool CreateClearUavHeap(ID3D12Device5* device);
    void CreateByteAddressSRV(ID3D12Device5* device, ID3D12Resource* buffer, UINT sizeInBytes, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle);

    bool m_initialized = false;
    UINT m_width = 0;
    UINT m_height = 0;

    std::unique_ptr<RayTracingPipelineState> m_pipelineState;
    std::unique_ptr<ShaderBindingTable> m_shaderBindingTable;

    ComPtr<ID3D12Resource> m_reflectionOutputResource;
    std::shared_ptr<DescriptorHandle> m_rtReflectionDescriptors;
    ComPtr<ID3D12DescriptorHeap> m_clearUavHeap;
    ComPtr<ID3D12Resource> m_constantBuffers[kFrameBufferCount];

    std::shared_ptr<RayTracedReflectionTarget> m_reflectionTarget;
    D3D12_RESOURCE_STATES m_reflectionOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    UINT m_cachedGeometryCount = 0;
    std::shared_ptr<DescriptorHandle> m_geometryVBIbDescriptors;

    bool m_debugGeometryColors = false;
    std::vector<ComPtr<ID3D12Resource>> m_debugColorTextures;
};
