#include "RayTracedAOManager.h"
#include "Renderer/Engine.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <Windows.h>
#include <stdio.h>

bool RayTracedAOManager::Init(ID3D12Device5* device, UINT width, UINT height)
{
    if (!device)
    {
        printf("[RayTracedAOManager] エラー: デバイスがnullです\n");
        return false;
    }

    m_width = width;
    m_height = height;

    m_pipelineState = std::make_unique<RayTracingPipelineState>();
    // TODO: ハードコーディングしているので修正する
    const std::wstring shaderPath = L"../EngineCore/src/Renderer/RayTracing/Shaders/RTAORayTracing.hlsl";

    if (!m_pipelineState->CreateForRTAO(device, shaderPath.c_str(), 8, 8))
    {
        printf("[RayTracedAOManager] エラー: RTAO Pipeline Stateの作成に失敗しました\n");
        return false;
    }

    m_shaderBindingTable = std::make_unique<ShaderBindingTable>();
    if (!m_shaderBindingTable->BuildForRTAO(device, m_pipelineState.get()))
    {
        printf("[RayTracedAOManager] エラー: RTAO SBTの作成に失敗しました\n");
        return false;
    }

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R32_FLOAT,
        width,
        height,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(m_aoOutputResource.GetAddressOf())
    ));
    m_aoOutputResource->SetName(L"RTAOOutput");
    m_aoOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    auto srvHandle = g_Engine->GetDescriptorHeap()->Allocate(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    device->CreateShaderResourceView(m_aoOutputResource.Get(), &srvDesc, srvHandle->cpuHandle);

    m_aoTarget = std::make_shared<RayTracedAOTarget>();
    m_aoTarget->SetResource(m_aoOutputResource);
    m_aoTarget->SetSrv(srvHandle);

    // エンジンヒープに TLAS SRV 用・AO UAV 用の2スロットを確保（CopyDescriptors のコピー元にしない）
    m_rtaoDescriptors = g_Engine->GetDescriptorHeap()->Allocate(2);
    if (!m_rtaoDescriptors)
    {
        printf("[RayTracedAOManager] エラー: エンジンヒープの Allocate(2) に失敗しました\n");
        return false;
    }
    const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE aoUavCpu = m_rtaoDescriptors->cpuHandle;
    aoUavCpu.ptr += descriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
    uavDesc.Texture2D.MipSlice = 0;
    device->CreateUnorderedAccessView(m_aoOutputResource.Get(), nullptr, &uavDesc, aoUavCpu);

    if (!CreateClearUavHeap(device))
    {
        printf("[RayTracedAOManager] エラー: ClearUAV用ヒープの作成に失敗しました\n");
        return false;
    }

    const UINT cbSize = (sizeof(RayTracedAOConstants) + 255) & ~255;
    auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
    for (UINT i = 0; i < kFrameBufferCount; ++i)
    {
        ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeap,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_constantBuffers[i].GetAddressOf())
        ));
    }

    m_initialized = true;
    return true;
}

RayTracedAORenderData RayTracedAOManager::GetRenderData(UINT frameIndex, AccelerationStructureManager* asManager) const
{
    RayTracedAORenderData data = {};
    if (!m_initialized || !asManager || !m_rtaoDescriptors)
        return data;
    const UINT index = frameIndex % kFrameBufferCount;
    const UINT descriptorSize = g_Engine->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    data.asManager = asManager;
    data.pipelineState = m_pipelineState.get();
    data.shaderBindingTable = m_shaderBindingTable.get();
    data.aoOutputResource = m_aoOutputResource.Get();
    data.descriptorHeap = g_Engine->GetDescriptorHeap()->GetHeap();
    data.tlasSrvCpuHandle = m_rtaoDescriptors->cpuHandle;
    data.tlasSrvGpuHandle = m_rtaoDescriptors->gpuHandle;
    data.aoUavGpuHandle = m_rtaoDescriptors->gpuHandle;
    data.aoUavGpuHandle.ptr += descriptorSize;
    data.clearUavCpuHandle = m_clearUavHeap
        ? m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
        : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    data.descriptorIncrementSize = descriptorSize;
    data.constantBuffer = m_constantBuffers[index].Get();
    data.pAoOutputState = const_cast<D3D12_RESOURCE_STATES*>(&m_aoOutputState);
    data.width = m_width;
    data.height = m_height;
    data.aoTarget = m_aoTarget;
    data.isValid = true;
    return data;
}

void RayTracedAOManager::UpdateConstants(const RayTracedAOConstants& constants, UINT frameIndex)
{
    const UINT index = frameIndex % kFrameBufferCount;
    ID3D12Resource* cb = m_constantBuffers[index].Get();
    if (!cb) return;
    void* mapped = nullptr;
    if (SUCCEEDED(cb->Map(0, nullptr, &mapped)))
    {
        memcpy(mapped, &constants, sizeof(RayTracedAOConstants));
        cb->Unmap(0, nullptr);
    }
}

void RayTracedAOManager::SetAoOutputState(D3D12_RESOURCE_STATES state)
{
    m_aoOutputState = state;
}

bool RayTracedAOManager::CreateClearUavHeap(ID3D12Device5* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_clearUavHeap.GetAddressOf())));
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
    uavDesc.Texture2D.MipSlice = 0;
    device->CreateUnorderedAccessView(
        m_aoOutputResource.Get(),
        nullptr,
        &uavDesc,
        m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
    );
    return true;
}
