#pragma once
#include <d3d12.h>
#include <vector>
#include "Modules/ComPtr.h"

class RtvDescriptorHeap
{
public:
    RtvDescriptorHeap();

    // 空いているディスクリプタのCPUハンドルを取得する
    D3D12_CPU_DESCRIPTOR_HANDLE GetNewHandle();

    // ヒープ本体を取得する
    ID3D12DescriptorHeap* GetHeap() { return m_pHeap.Get(); }

private:
    ComPtr<ID3D12DescriptorHeap> m_pHeap;
    UINT m_IncrementSize; // ハンドル1つ分のサイズ
    UINT m_CurrentHandleIndex; // 次に割り当てるハンドルのインデックス
};