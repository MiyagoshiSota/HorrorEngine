#include "CbvDescriptorHeap.h"
#include "Renderer/Engine.h" // g_Engine を使うため

// CbvDescriptorHeap::CbvDescriptorHeap(UINT numDescriptors) 
//     : m_NumDescriptors(numDescriptors), m_NextAvailableIndex(0)
// {
//     auto device = g_Engine->Device();
//
//     D3D12_DESCRIPTOR_HEAP_DESC desc = {};
//     desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // CBV, SRV, UAVは同じヒープタイプ
//     desc.NumDescriptors = m_NumDescriptors;
//     desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // シェーダーから見えるように
//     desc.NodeMask = 0;
//
//     HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pHeap));
//     if (FAILED(hr)) {
//         // 本来は例外を投げるなど、より堅牢なエラー処理が望ましい
//         printf("Failed to create CBV descriptor heap.");
//         throw std::runtime_error("Failed to create CBV descriptor heap.");
//     }
//
//     m_IncrementSize = device->GetDescriptorHandleIncrementSize(desc.Type);
// }
//
// std::shared_ptr<DescriptorHandle> CbvDescriptorHeap::Allocate()
// {
//     std::lock_guard<std::mutex> lock(m_Mutex); // スレッドセーフのためのロック
//
//     UINT index;
//     if (!m_FreeList.empty()) {
//         index = m_FreeList.back();
//         m_FreeList.pop_back();
//     } else {
//         index = m_NextAvailableIndex;
//         m_NextAvailableIndex++;
//     }
//
//     if (index >= m_NumDescriptors) {
//         printf("CBV descriptor heap is full.\n");
//         return nullptr;
//     }
//
//     auto handle = new DescriptorHandle();
//     handle->index = index;
//     handle->pOwnerHeap = this; // pOwnerHeapの型を基底クラスなどにするか、工夫が必要
//
//     // CPUハンドルを計算
//     handle->cpuHandle = m_pHeap->GetCPUDescriptorHandleForHeapStart();
//     handle->cpuHandle.ptr += index * m_IncrementSize;
//
//     // GPUハンドルを計算
//     handle->gpuHandle = m_pHeap->GetGPUDescriptorHandleForHeapStart();
//     handle->gpuHandle.ptr += index * m_IncrementSize;
//     
//     // カスタムデリータ付きのshared_ptrを作成して返す
//     return {
//         handle,
//         [this](DescriptorHandle* ptr) {
//             if (ptr) {
//                 this->Free(ptr->index);
//                 delete ptr;
//             }
//         }
//     };
// }
//
// ID3D12DescriptorHeap* CbvDescriptorHeap::GetHeap() const
// {
//     return m_pHeap.Get();
// }
//
// void CbvDescriptorHeap::Free(UINT index)
// {
//     std::lock_guard<std::mutex> lock(m_Mutex);
//     m_FreeList.push_back(index);
// }