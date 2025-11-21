#pragma once
#include <d3d12.h>
#include <memory>

#include "Modules/ComPtr.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"

class ITargetBase
{
public:
	virtual ~ITargetBase() = default;
	// DSV用
    virtual void Create(
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
    ) = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return m_hDsv; }

    // RTV用
    virtual void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT16 array_size,
        UINT16 mip_levels,
        UINT sample_count,
        UINT sample_quality,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    )= 0;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return m_hRtv; }

    // SRV用
    std::shared_ptr<DescriptorHandle> GetSRVHandle() const { return m_hSrv; }
    
public:
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_CurrentState = state; }
    ComPtr<ID3D12Resource> GetResource() const { return m_pResource.Get(); }
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentState; }

protected:
    ComPtr<ID3D12Resource> m_pResource;
    D3D12_RESOURCE_STATES m_CurrentState;

    D3D12_CPU_DESCRIPTOR_HANDLE m_hRtv; // RTVのCPUハンドル
    std::shared_ptr<DescriptorHandle> m_hSrv; // SRVのCPUハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE m_hDsv; // DSVのCPUハンドル

    UINT m_Width;
    UINT m_Height;
};
