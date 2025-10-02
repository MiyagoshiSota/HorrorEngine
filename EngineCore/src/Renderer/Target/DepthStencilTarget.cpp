#include "DepthStencilTarget.h"

#include <d3dx12.h>

DepthStencilTarget::DepthStencilTarget()
{
}

void DepthStencilTarget::Create(
    ID3D12Device* pDevice,
    UINT width,
    UINT height,
    DXGI_FORMAT resourceFormat,
    DXGI_FORMAT dsvFormat,
    DXGI_FORMAT srvFormat,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle)
{
    m_Width = width;
    m_Height = height;
    
    m_hDsv = dsvHandle;
    m_hSrv = srvHandle;

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        resourceFormat,
        m_Width,
        m_Height,
        1, // ArraySize
        1, // MipLevels
        1, // SampleCount
        0, // SampleQuality
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL // レンダーターゲットとして使うことを許可
    );

    // Clear
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = dsvFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // ヒープのプロパティを設定
    auto heapProps =CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    // リソースを生成
    HRESULT hr = pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_pResource)
        );
    if (FAILED(hr)) {
        // エラー処理
        return;
    }
    m_CurrentState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    // DSVを生成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = dsvFormat;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    pDevice->CreateDepthStencilView(m_pResource.Get(),&dsvDesc,m_hDsv);

    // SRVを生成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = srvFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    pDevice->CreateShaderResourceView(m_pResource.Get(),&srvDesc,m_hSrv);
}

void DepthStencilTarget::Create(ID3D12Device* pDevice, UINT width, UINT height, DXGI_FORMAT format,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle)
{
}
