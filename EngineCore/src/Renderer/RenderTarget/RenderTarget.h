#pragma once

#include <d3d12.h>
#include <wrl.h> // ComPtrを使うため
#include <string>

// d3dx12.h のヘルパー構造体を使うと便利
#include <d3dx12.h>

class RenderTarget
{
public:
    // コンストラクタ
    RenderTarget();

    // レンダーターゲットを生成する
    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle
    );

    // --- ゲッター関数 ---
    ID3D12Resource* GetResource() const { return m_pResource.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return m_hRtv; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return m_hSrv; }
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentState; }

    // --- 状態管理 ---
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_CurrentState = state; }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hRtv; // RTVのCPUハンドル
    D3D12_CPU_DESCRIPTOR_HANDLE m_hSrv; // SRVのCPUハンドル
    D3D12_RESOURCE_STATES m_CurrentState;

    UINT m_Width;
    UINT m_Height;
    DXGI_FORMAT m_Format;
};