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
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandleStart,
    std::shared_ptr<DescriptorHandle> srvHandle)
{
    m_Width = width;
    m_Height = height;
    
    m_hDsv = dsvHandleStart;
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
        IID_PPV_ARGS(m_pResource.ReleaseAndGetAddressOf())
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

    // DSVハンドルのサイズ取得 (オフセット計算用)
    UINT dsvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    D3D12_CPU_DESCRIPTOR_HANDLE currentDSVHandle = dsvHandleStart;

    if (array_size > 1)
    {
        // Texture2DArray の場合
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.ArraySize = 1; 

        for (UINT i = 0; i < array_size; ++i)
        {
            // i番目のスライスを指す
            dsvDesc.Texture2DArray.FirstArraySlice = i;

            // DSV作成
            pDevice->CreateDepthStencilView(m_pResource.Get(), &dsvDesc, currentDSVHandle);

            // ハンドルを次へ進める
            currentDSVHandle.ptr += dsvDescriptorSize;
        }
    }
    else
    {
        // 通常の単一テクスチャ (またはMSAA)
        if (sample_count > 1)
        {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;
        }

        // 1回だけ作成
        pDevice->CreateDepthStencilView(m_pResource.Get(), &dsvDesc, m_hDsv);
    }

    // SRVの生成
    if (m_hSrv)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = srvFormat;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		// array_sizeとsample_countに応じてViewDimensionを設定
        if (array_size > 1)
        {
            if (sample_count > 1)
            {
                // 配列かつMSAA有効時
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                srvDesc.Texture2DMSArray.ArraySize = array_size;
            }
            else
            {
                // 配列かつ通常時
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = mip_levels;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = array_size;
                srvDesc.Texture2DArray.PlaneSlice = 0;
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            }
        }
        else
        {
            if (sample_count > 1)
            {
                // 単一テクスチャかつMSAA有効時
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                // MSAAテクスチャはミップマップを持てない
            }
            else
            {
                // 単一テクスチャかつ通常時
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = mip_levels;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			}
		}

        pDevice->CreateShaderResourceView(m_pResource.Get(), &srvDesc, m_hSrv->cpuHandle);
    }
}
