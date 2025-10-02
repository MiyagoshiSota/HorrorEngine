#pragma once
#include <d3d12.h>

#include "Modules/ComPtr.h"

class ITargetBase
{
public:
    // DSV用
    virtual void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT resourceFormat,
        DXGI_FORMAT dsvFormat,
        DXGI_FORMAT srvFormat,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle
    ) = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return m_hDsv; }

    // RTV用
    virtual void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle
    )= 0;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return m_hRtv; }

    // SRV用
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return m_hSrv; }
    
public:
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_CurrentState = state; }
    ComPtr<ID3D12Resource> GetResource() const { return m_pResource.Get(); }
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentState; }

protected:
    ComPtr<ID3D12Resource> m_pResource;
    D3D12_RESOURCE_STATES m_CurrentState;

    D3D12_CPU_DESCRIPTOR_HANDLE m_hRtv; // RTVのCPUハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE m_hSrv; // SRVのCPUハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE m_hDsv; // DSVのCPUハンドル

    UINT m_Width;
    UINT m_Height;
};
