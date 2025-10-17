#pragma once
#include <d3d12.h>
#include <memory>

#include "CbvDescriptorHeap.h"

class SrvDescriptorHeap;

struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
    UINT index = 0;
    SrvDescriptorHeap* pOwnerHeap = nullptr;
};
