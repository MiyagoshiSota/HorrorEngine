#pragma once
#include <d3d12.h>
#include <vector>
#include <string>
#include <memory>
#include "Modules/ComPtr.h"

/// Ray Tracing Pipeline State Object
/// DXRパイプラインの構築と管理を行う
class RayTracingPipelineState
{
public:
    RayTracingPipelineState() = default;
    ~RayTracingPipelineState() = default;

    /// パイプラインを構築（Shadow用）
    bool Create(
        ID3D12Device5* device,
        const wchar_t* shaderLibraryPath,
        UINT maxPayloadSize = 32,
        UINT maxAttributeSize = 8,
        UINT maxRecursionDepth = 1
    );

    /// RTAO用パイプラインを構築（別HLSL・別ルートシグネチャ）
    bool CreateForRTAO(
        ID3D12Device5* device,
        const wchar_t* shaderLibraryPath,
        UINT maxPayloadSize = 8,
        UINT maxAttributeSize = 8
    );

    /// RTGI用パイプラインを構築（1-bounce間接光）
    bool CreateForRTGI(
        ID3D12Device5* device,
        const wchar_t* shaderLibraryPath,
        UINT maxPayloadSize = 16,
        UINT maxAttributeSize = 8
    );

    /// RT Reflection用パイプラインを構築（鏡面反射レイ）
    bool CreateForRTReflection(
        ID3D12Device5* device,
        const wchar_t* shaderLibraryPath,
        UINT maxPayloadSize = 12,
        UINT maxAttributeSize = 8
    );

    ID3D12StateObject* GetStateObject() const { return m_stateObject.Get(); }
    ID3D12StateObjectProperties* GetStateObjectProperties() const
    {
        return m_stateObjectProperties.Get();
    }
    /// グローバルルートシグネチャ（コマンドリストに SetGraphicsRootSignature で設定する用）
    ID3D12RootSignature* GetGlobalRootSignature() const { return m_globalRootSignature.Get(); }

    /// シェーダー識別子を取得
    void* GetShaderIdentifier(const wchar_t* shaderName) const;

private:
    ComPtr<ID3D12StateObject> m_stateObject;
    ComPtr<ID3D12StateObjectProperties> m_stateObjectProperties;
    ComPtr<ID3D12RootSignature> m_globalRootSignature;
    ComPtr<ID3D12RootSignature> m_localRootSignature;  // DXR Local Root Signature (State Object 用、export に紐付け)
};

/// Shader Binding Table (SBT)
/// レイトレーシングシェーダーのバインディング情報を管理
class ShaderBindingTable
{
public:
    ShaderBindingTable() = default;
    ~ShaderBindingTable() = default;

    /// SBTを構築（Shadow用）
    bool Build(
        ID3D12Device5* device,
        RayTracingPipelineState* pipelineState,
        UINT numRayGenShaders,
        UINT numMissShaders,
        UINT numHitGroups
    );

    /// RTAO用SBTを構築（RayGen/Miss/HitGroup 各1）
    bool BuildForRTAO(ID3D12Device5* device, RayTracingPipelineState* pipelineState);

    /// RTGI用SBTを構築（RayGen/Miss 各1、HitGroup は numHitGroups 件・各レコードに VB/IB ディスクリプタ先頭を渡す）
    bool BuildForRTGI(ID3D12Device5* device, RayTracingPipelineState* pipelineState);
    bool BuildForRTGI(
        ID3D12Device5* device,
        RayTracingPipelineState* pipelineState,
        UINT numHitGroupRecords,
        D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptorForVBIB,
        UINT descriptorIncrementSize);

    /// RT Reflection用SBTを構築（RTGIと同様に HitGroup  per geometry）
    bool BuildForRTReflection(ID3D12Device5* device, RayTracingPipelineState* pipelineState);
    bool BuildForRTReflection(
        ID3D12Device5* device,
        RayTracingPipelineState* pipelineState,
        UINT numHitGroupRecords,
        D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptorForVBIB,
        UINT descriptorIncrementSize);

    /// ディスパッチ記述子を取得
    D3D12_DISPATCH_RAYS_DESC GetDispatchRaysDesc(UINT width, UINT height) const;

private:
    /// レコードをSBTに追加
    void AddShaderRecord(
        void* destination,
        void* shaderIdentifier,
        UINT shaderIdentifierSize,
        void* localRootArguments = nullptr,
        UINT localRootArgumentsSize = 0
    );

private:
    ComPtr<ID3D12Resource> m_rayGenShaderTable;
    ComPtr<ID3D12Resource> m_missShaderTable;
    ComPtr<ID3D12Resource> m_hitGroupShaderTable;

    UINT m_shaderRecordSize;
    UINT m_rayGenShaderRecordSize;
    UINT m_missShaderRecordSize;
    UINT m_hitGroupShaderRecordSize;

    UINT m_numRayGenShaders;
    UINT m_numMissShaders;
    UINT m_numHitGroups;
};
