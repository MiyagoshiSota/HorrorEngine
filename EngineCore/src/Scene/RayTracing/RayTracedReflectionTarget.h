#pragma once
#include "Renderer/Target/ITargetBase.h"
#include "Modules/ComPtr.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"
#include <memory>

/// RT Reflection 出力を LightingPass に渡すためのラッパー（RGB 反射カラー）
class RayTracedReflectionTarget : public ITargetBase
{
public:
    RayTracedReflectionTarget() = default;

    void SetResource(ComPtr<ID3D12Resource> resource);
    void SetSrv(std::shared_ptr<DescriptorHandle> srv);

    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT resourceFormat,
        DXGI_FORMAT dsvFormat,
        DXGI_FORMAT srvFormat,
        UINT16 array_size,
        UINT16 mip_levels,
        UINT sample_count,
        UINT sample_quality,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    ) override {}

    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT16 array_size,
        UINT16 mip_levels,
        UINT sample_count,
        UINT sample_quality,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    ) override {}
};
