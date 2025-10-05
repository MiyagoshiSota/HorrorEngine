#pragma once
#include <d3d12.h>

#include "ITargetBase.h"

class DepthStencilTarget : public ITargetBase
{
public:
    DepthStencilTarget();

    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT resourceFormat,
        DXGI_FORMAT dsvFormat,
        DXGI_FORMAT srvFormat,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        std::shared_ptr<DescriptorHandle>  srvHandle
    ) override;

    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    ) override;
};
