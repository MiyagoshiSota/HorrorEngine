#include "RenderTarget.h"
#include "Renderer/Engine.h" // g_Engine を使う場合

void RenderTarget::Create(
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
)
{
    m_Width = width;
    m_Height = height;
    m_Format = format;
    m_hRtv = rtvHandle;
    m_hSrv = srvHandle;

    // リソースの設定を定義
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        m_Format,
        m_Width,
        m_Height,
        array_size, // ArraySize 1
        mip_levels, // MipLevels 1
        sample_count, // SampleCount 1
        sample_quality, // SampleQuality 0
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET // レンダーターゲットとして使うことを許可
    );

    // クリア時の色を設定
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = m_Format;
    clearValue.Color[0] = 0.0f; // R
    clearValue.Color[1] = 0.0f; // G
    clearValue.Color[2] = 0.0f; // B
    clearValue.Color[3] = 0.0f; // A

    // ヒープのプロパティを設定
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    // リソースを生成
    HRESULT hr = pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET, // 初期状態はレンダーターゲット
        &clearValue,
        IID_PPV_ARGS(&m_pResource)
    );
    if (FAILED(hr)) {
        // エラー処理
        return;
    }
    m_CurrentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

    // レンダーターゲットビュー (RTV) を生成
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = m_Format;
    if (sample_count > 1)
    {
		// MSAA対応
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
		// 通常の2Dテクスチャ
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
    }
	pDevice->CreateRenderTargetView(m_pResource.Get(), &rtvDesc, m_hRtv);

    // シェーダーリソースビュー (SRV) を生成
    if (m_hSrv)
    {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = m_Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (sample_count > 1)
        {
			// MSAA対応
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
			// 通常の2Dテクスチャ
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = mip_levels;
        }
		pDevice->CreateShaderResourceView(m_pResource.Get(), &srvDesc, m_hSrv->cpuHandle);
    }
}