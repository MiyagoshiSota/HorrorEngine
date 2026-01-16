#include "DescriptorHeap.h"
#include "Renderer/Texture/Texture2D.h"
#include <d3dx12.h>
#include "Renderer/Engine.h"
#include "Modules/ComPtr.h"

const UINT HANDLE_MAX = 512;

DescriptorHeap::DescriptorHeap(UINT numDescriptors) : m_NumDescriptors(numDescriptors) , m_NextAvailableIndex(0)
{
	auto device = g_Engine->Device();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = m_NumDescriptors;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;

	// SRV用のヒープを生成
	HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pHeap));
	if (FAILED(hr)) {
		printf("Failed to create SRV descriptor heap.");
	}

	m_IncrementSize = device->GetDescriptorHandleIncrementSize(desc.Type);
}

// 引数に count を追加 (デフォルトは 1)
std::shared_ptr<DescriptorHandle> DescriptorHeap::Allocate(UINT count)
{
    UINT index = -1;

    // 1個だけの確保なら、FreeList（再利用）をチェックする
    if (count == 1 && !m_FreeList.empty()) {
        index = m_FreeList.back();
        m_FreeList.pop_back();
    }
    else {
        // 複数個の確保、またはFreeListが空なら、末尾から確保する

        // 容量チェック
        if (m_NextAvailableIndex + count > m_NumDescriptors) {
            return nullptr; // 足りない
        }

        index = m_NextAvailableIndex;
        m_NextAvailableIndex += count; // 指定された個数分進める！
    }

    // ハンドルを作成
    auto handle = std::make_shared<DescriptorHandle>();
    handle->index = index;
    handle->count = count; // ★重要：何個確保したか覚えておく必要がある

    // CPUハンドルを計算 (開始位置)
    handle->cpuHandle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
    handle->cpuHandle.ptr += index * m_IncrementSize;

    // GPUハンドルを計算 (開始位置)
    handle->gpuHandle = m_pHeap->GetGPUDescriptorHandleForHeapStart();
    handle->gpuHandle.ptr += index * m_IncrementSize;

    // カスタムデリータを設定 (ラムダ式などで、解放時に count 分を Free する処理が必要)
    // handle_ptr.reset(handle, [this](DescriptorHandle* h) { this->Free(h); }); のようなイメージ

    return handle;
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap() const
{
	return m_pHeap.Get();
}

void DescriptorHeap::Free(std::shared_ptr<DescriptorHandle> handle)
{
    // TODO: Freeよばないと
    if (handle->count == 1) {
        m_FreeList.push_back(handle->index);
    }
}