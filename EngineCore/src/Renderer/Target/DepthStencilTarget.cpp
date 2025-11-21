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
    UINT16 array_size,
    UINT16 mip_levels,
    UINT sample_count,
    UINT sample_quality,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
    std::shared_ptr<DescriptorHandle> srvHandle)
{
    m_Width = width;
    m_Height = height;
    
    m_hDsv = dsvHandle;
    m_hSrv = srvHandle;

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        resourceFormat,
        m_Width,
        m_Height,
        array_size, // ArraySize
        mip_levels, // MipLevels
        sample_count, // SampleCount
        sample_quality, // SampleQuality
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL // レンダーターゲットとして使うことを許可
    );

    // Clear
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = dsvFormat;
    clearValue.DepthStencil.Depth = 1.0f;
    clearValue.DepthStencil.Stencil = 0;

    // ヒープのプロパティを設定
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

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

    // DSVの生成
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = dsvFormat;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    if (sample_count > 1)
    {
        // MSAA有効時
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        // TEXTURE2DMSには MipSlice は存在しないので設定不要
    }
    else
    {
        // 通常時
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
    }

    pDevice->CreateDepthStencilView(m_pResource.Get(), &dsvDesc, m_hDsv);

    // SRVの生成
    if (m_hSrv)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = srvFormat;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (sample_count > 1)
        {
            // MSAA有効時
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            // MSAAテクスチャはミップマップを持てない
        }
        else
        {
            // 通常時
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
        }

        pDevice->CreateShaderResourceView(m_pResource.Get(), &srvDesc, m_hSrv->cpuHandle);
    }
}
