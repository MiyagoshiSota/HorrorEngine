#pragma once
#include "Modules/ComPtr.h"
#include <d3dx12.h>
#include <mutex>
#include <vector>
#include "DescriptorHandle.h" // SrvDescriptorHeapと共通のハンドル構造体を使用

class CbvDescriptorHeap
{
// public:
//     CbvDescriptorHeap(UINT numDescriptors);
//     ~CbvDescriptorHeap() = default;
//
//     // 新しいディスクリプタハンドルを確保し、自動解放機能付きのshared_ptrとして返す
//     std::shared_ptr<DescriptorHandle> Allocate();
//     
//     // ディスクリプタヒープ本体を返す
//     ID3D12DescriptorHeap* GetHeap() const;
//
// private:
//     // shared_ptrのデリータから呼ばれる解放処理
//     void Free(UINT index);
//     
//     ComPtr<ID3D12DescriptorHeap> m_pHeap;
//     UINT m_NumDescriptors;
//     UINT m_IncrementSize;
//     UINT m_NextAvailableIndex;
//     std::vector<UINT> m_FreeList;
//     std::mutex m_Mutex; // スレッドセーフのためのミューテックス
};