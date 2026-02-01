#pragma once
#include <memory>
#include <d3d12.h>
#include <DirectXMath.h>
#include "Modules/ComPtr.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Renderer/RayTracing/RayTracingPipelineState.h"
#include "RayTracedShadowMapTarget.h"

struct ID3D12Device5;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;

/// Ray Traced Shadow用シーン定数（シャドウマップ方式：invLightViewProj で texel→ワールド）
struct RayTracedShadowSceneConstants
{
    DirectX::XMFLOAT3 lightPosition;
    float lightRadius;
    DirectX::XMFLOAT3 lightDirection;
    float padding;
    DirectX::XMFLOAT4X4 invLightViewProj;
};

/// Passに渡す描画用データ
struct RayTracedShadowRenderData
{
    AccelerationStructureManager* asManager = nullptr;
    RayTracingPipelineState* pipelineState = nullptr;
    ShaderBindingTable* shaderBindingTable = nullptr;
    ID3D12Resource* shadowOutputResource = nullptr;
    ID3D12DescriptorHeap* descriptorHeap = nullptr;
    /// ClearUnorderedAccessViewFloat 用。CPU ハンドルは非シェーダー可視ヒープのディスクリプタを指す必要がある。
    D3D12_CPU_DESCRIPTOR_HANDLE clearUavCpuHandle = { 0 };
    ID3D12Resource* sceneConstantBuffer = nullptr;
    D3D12_RESOURCE_STATES* pShadowOutputState = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool isValid = false;
    std::shared_ptr<RayTracedShadowMapTarget> shadowMapTarget; // メインパスで ShadowMap としてバインドする用
};

/// Ray Traced Shadowのリソース管理を行うマネージャー
/// AS/RTPSO/SBT/UAV/ディスクリプタヒープ/定数バッファの所有と、Pass用データの提供を担当
class RayTracedShadowManager
{
public:
    RayTracedShadowManager() = default;
    ~RayTracedShadowManager() = default;

    /// リソースを初期化（デバイス・解像度）
    bool Init(ID3D12Device5* device, UINT width, UINT height);

    /// Passに渡す描画データを取得
    RayTracedShadowRenderData GetRenderData() const;

    /// シーン定数バッファを更新（Passからコールバック経由で呼ばれる）
    void UpdateSceneConstants(const RayTracedShadowSceneConstants& constants);

    /// シャドウ出力リソースの現在状態を更新（Passがバリア後に呼ぶ）
    void SetShadowOutputState(D3D12_RESOURCE_STATES state);

    /// 有効かどうか
    bool IsValid() const { return m_initialized; }

private:
    bool CreateDescriptorHeap(ID3D12Device5* device);
    /// ClearUnorderedAccessViewFloat の第2引数用。非シェーダー可視ヒープに UAV を1つ作成する。
    bool CreateClearUavHeap(ID3D12Device5* device);

private:
    bool m_initialized = false;
    UINT m_width = 0;
    UINT m_height = 0;

    std::unique_ptr<AccelerationStructureManager> m_asManager;
    std::unique_ptr<RayTracingPipelineState> m_pipelineState;
    std::unique_ptr<ShaderBindingTable> m_shaderBindingTable;

    ComPtr<ID3D12Resource> m_shadowOutputResource;
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    ComPtr<ID3D12DescriptorHeap> m_clearUavHeap;  // ClearUAV 用 CPU ハンドル（非シェーダー可視）
    ComPtr<ID3D12Resource> m_sceneConstantBuffer;
    std::shared_ptr<RayTracedShadowMapTarget> m_shadowMapTarget;

    D3D12_RESOURCE_STATES m_shadowOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
};
