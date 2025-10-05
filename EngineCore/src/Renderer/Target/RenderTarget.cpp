#include "RenderTarget.h"
#include "Renderer/Engine.h" // g_Engine を使う場合

void RenderTarget::Create(
    ID3D12Device* pDevice,
    UINT width,
    UINT height,
    DXGI_FORMAT format,
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
        1, // ArraySize
        1, // MipLevels
        1, // SampleCount
        0, // SampleQuality
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET // レンダーターゲットとして使うことを許可
    );

    // クリア時の色を設定
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = m_Format;
    clearValue.Color[0] = 1.0f; // R
    clearValue.Color[1] = 1.0f; // G
    clearValue.Color[2] = 1.0f; // B
    clearValue.Color[3] = 1.0f; // A

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
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    pDevice->CreateRenderTargetView(m_pResource.Get(), &rtvDesc, m_hRtv);

    // シェーダーリソースビュー (SRV) を生成
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = m_Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    pDevice->CreateShaderResourceView(m_pResource.Get(), &srvDesc, m_hSrv->cpuHandle);
}