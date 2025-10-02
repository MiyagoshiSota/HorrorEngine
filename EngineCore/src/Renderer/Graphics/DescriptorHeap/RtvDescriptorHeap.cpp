#include "RtvDescriptorHeap.h"
#include "Renderer/Engine.h" // g_Engine を使うため

const UINT RTV_HANDLE_MAX = 256; // RTVはSRVほど多くは必要ないことが多い

RtvDescriptorHeap::RtvDescriptorHeap()
    : m_CurrentHandleIndex(0)
{
    auto device = g_Engine->Device();

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = RTV_HANDLE_MAX;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    // RTV用ディスクリプタヒープを生成
    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pHeap));
    if (FAILED(hr)) {
        // エラー処理
        return;
    }

    m_IncrementSize = device->GetDescriptorHandleIncrementSize(desc.Type);
}

D3D12_CPU_DESCRIPTOR_HANDLE RtvDescriptorHeap::GetNewHandle()
{
    if (m_CurrentHandleIndex >= RTV_HANDLE_MAX) {
        return {}; 
    }

    // ヒープの先頭アドレスを取得
    auto handle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
    
    // 現在のインデックス分だけアドレスを進める
    handle.ptr += m_CurrentHandleIndex * m_IncrementSize;

    // 次に使うインデックスを1つ進める
    m_CurrentHandleIndex++;

    return handle;
}