#pragma once
#include <d3d12.h>

#include "ITargetBase.h"

class DepthStencilTarget : public ITargetBase
{
public:
    DepthStencilTarget();

	// 生成
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
        std::shared_ptr<DescriptorHandle>  srvHandle
    ) override;

	// RTV用のCreateは使わない
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
    ) override{};
};
