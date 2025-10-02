#include "SrvDescriptorHeap.h"
#include "Renderer/Texture/Texture2D.h"
#include <d3dx12.h>
#include "Renderer/Engine.h"
#include "Modules/ComPtr.h"

const UINT HANDLE_MAX = 512;

SrvDescriptorHeap::SrvDescriptorHeap(UINT numDescriptors) : m_NumDescriptors(numDescriptors) , m_NextAvailableIndex(0)
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

std::shared_ptr<DescriptorHandle> SrvDescriptorHeap::Allocate()
{
	UINT index;
	if (!m_FreeList.empty()) {
		index = m_FreeList.back();
		m_FreeList.pop_back();
	} else {
		index = m_NextAvailableIndex;
		m_NextAvailableIndex++;
	}

	if (index >= m_NumDescriptors) {
		// 確保失敗
		return nullptr;
	}

	// ハンドルを作成
	auto handle = new DescriptorHandle();
	handle->index = index;
	handle->pOwnerHeap = this;

	// CPUハンドルを計算
	handle->cpuHandle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
	handle->cpuHandle.ptr += index * m_IncrementSize;

	// GPUハンドルを計算
	handle->gpuHandle = m_pHeap->GetGPUDescriptorHandleForHeapStart();
	handle->gpuHandle.ptr += index * m_IncrementSize;
    
	// カスタムデリータ付きのshared_ptrを作成して返す
	// これにより、このshared_ptrの参照が全て無くなった時に自動でFreeが呼ばれる
	return std::shared_ptr<DescriptorHandle>(
		handle,
		[this](DescriptorHandle* ptr) {
			this->Free(ptr->index);
			delete ptr;
		}
	);
}

ID3D12DescriptorHeap* SrvDescriptorHeap::GetHeap() const
{
	return m_pHeap.Get();
}

void SrvDescriptorHeap::Free(UINT index)
{
	// インデックスをフリーリストに戻す
	m_FreeList.push_back(index);
}