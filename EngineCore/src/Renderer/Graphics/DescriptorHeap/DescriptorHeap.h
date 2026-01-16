#pragma once
#include "Modules/ComPtr.h"
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

	std::shared_ptr<DescriptorHandle> Allocate(UINT count);
	
	ID3D12DescriptorHeap* GetHeap() const; // ディスクリプタヒープを返す
	UINT GetIncrementSize() { return m_IncrementSize; }
	
private:
	void Free(std::shared_ptr<DescriptorHandle> handle);
	
	ComPtr<ID3D12DescriptorHeap> m_pHeap;
	UINT m_NumDescriptors;
	UINT m_IncrementSize;
	UINT m_NextAvailableIndex;
	std::vector<UINT> m_FreeList;
};

