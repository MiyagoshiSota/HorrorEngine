#pragma once
#include <d3d12.h>

struct DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
    UINT index = 0;
};