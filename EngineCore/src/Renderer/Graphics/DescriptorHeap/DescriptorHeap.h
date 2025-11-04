#pragma once
#include "Modules/ComPtr.h"
#include <d3dx12.h>
#include <mutex>
#include <vector>
#include "DescriptorHandle.h"

class ConstantBuffer;
class Texture2D;

class DescriptorHeap
{
public:
	DescriptorHeap(UINT numDescriptors);
	~DescriptorHeap() = default;

	std::shared_ptr<DescriptorHandle> Allocate();
	
	ID3D12DescriptorHeap* GetHeap() const; // ディスクリプタヒープを返す
	
private:
	void Free(UINT index);
	
	ComPtr<ID3D12DescriptorHeap> m_pHeap;
	UINT m_NumDescriptors;
	UINT m_IncrementSize;
	UINT m_NextAvailableIndex;
	std::vector<UINT> m_FreeList;
};

