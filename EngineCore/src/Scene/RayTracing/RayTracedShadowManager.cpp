#include "RayTracedShadowManager.h"
#include "Renderer/Engine.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <Windows.h>
#include <stdio.h>

bool RayTracedShadowManager::Init(ID3D12Device5* device, UINT width, UINT height)
{
    if (!device)
    {
        printf("[RayTracedShadowManager] エラー: デバイスがnullです\n");
        return false;
    }

    printf("[RayTracedShadowManager] 初期化開始 (解像度: %ux%u)\n", width, height);

    m_width = width;
    m_height = height;

    m_asManager = std::make_unique<AccelerationStructureManager>();

    m_pipelineState = std::make_unique<RayTracingPipelineState>();

    wchar_t currentDir[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDir);
    printf("[RayTracedShadowManager] カレントディレクトリ: %ls\n", currentDir);

    std::wstring shaderPath = L"../EngineCore/src/Renderer/RayTracing/Shaders/ShadowRayTracing.hlsl";
    printf("[RayTracedShadowManager] シェーダーパス: %ls\n", shaderPath.c_str());

    if (!m_pipelineState->Create(device, shaderPath.c_str()))
    {
        printf("[RayTracedShadowManager] エラー: Ray Tracing Pipeline Stateの作成に失敗しました\n");
        return false;
    }

    m_shaderBindingTable = std::make_unique<ShaderBindingTable>();
    if (!m_shaderBindingTable->Build(device, m_pipelineState.get(), 1, 1, 1))
    {
        printf("[RayTracedShadowManager] エラー: Shader Binding Tableの作成に失敗しました\n");
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
        IID_PPV_ARGS(m_shadowOutputResource.GetAddressOf())
    ));

    m_shadowOutputResource->SetName(L"RayTracedShadowOutput");
    m_shadowOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    // メインパスで ShadowMap としてバインドするための SRV をエンジンヒープに作成
    auto srvHandle = g_Engine->GetDescriptorHeap()->Allocate(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    device->CreateShaderResourceView(m_shadowOutputResource.Get(), &srvDesc, srvHandle->cpuHandle);

    m_shadowMapTarget = std::make_shared<RayTracedShadowMapTarget>();
    m_shadowMapTarget->SetResource(m_shadowOutputResource);
    m_shadowMapTarget->SetSrv(srvHandle);

    if (!CreateDescriptorHeap(device))
    {
        printf("[RayTracedShadowManager] エラー: ディスクリプタヒープの作成に失敗しました\n");
        return false;
    }
    if (!CreateClearUavHeap(device))
    {
        printf("[RayTracedShadowManager] エラー: ClearUAV用ヒープの作成に失敗しました\n");
        return false;
    }

    auto heapProps2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        (sizeof(RayTracedShadowSceneConstants) + 255) & ~255
    );

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps2,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_sceneConstantBuffer.GetAddressOf())
    ));

    m_initialized = true;
    printf("[RayTracedShadowManager] ===== Ray Traced Shadow Manager 初期化成功 =====\n");
    return true;
}

RayTracedShadowRenderData RayTracedShadowManager::GetRenderData() const
{
    RayTracedShadowRenderData data = {};
    if (!m_initialized)
    {
        return data;
    }
    data.asManager = m_asManager.get();
    data.pipelineState = m_pipelineState.get();
    data.shaderBindingTable = m_shaderBindingTable.get();
    data.shadowOutputResource = m_shadowOutputResource.Get();
    data.descriptorHeap = m_descriptorHeap.Get();
    data.clearUavCpuHandle = m_clearUavHeap
        ? m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
        : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    data.sceneConstantBuffer = m_sceneConstantBuffer.Get();
    data.pShadowOutputState = const_cast<D3D12_RESOURCE_STATES*>(&m_shadowOutputState);
    data.width = m_width;
    data.height = m_height;
    data.shadowMapTarget = m_shadowMapTarget;
    data.isValid = true;
    return data;
}

void RayTracedShadowManager::UpdateSceneConstants(const RayTracedShadowSceneConstants& constants)
{
    if (!m_sceneConstantBuffer)
    {
        return;
    }
    void* mappedData = nullptr;
    if (SUCCEEDED(m_sceneConstantBuffer->Map(0, nullptr, &mappedData)))
    {
        memcpy(mappedData, &constants, sizeof(RayTracedShadowSceneConstants));
        m_sceneConstantBuffer->Unmap(0, nullptr);
    }
}

void RayTracedShadowManager::SetShadowOutputState(D3D12_RESOURCE_STATES state)
{
    m_shadowOutputState = state;
}

bool RayTracedShadowManager::CreateDescriptorHeap(ID3D12Device5* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    ThrowIfFailed(device->CreateDescriptorHeap(
        &heapDesc,
        IID_PPV_ARGS(m_descriptorHeap.GetAddressOf())
    ));

    return true;
}

bool RayTracedShadowManager::CreateClearUavHeap(ID3D12Device5* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // 非シェーダー可視。ClearUnorderedAccessViewFloat の CPU ハンドル用

    ThrowIfFailed(device->CreateDescriptorHeap(
        &heapDesc,
        IID_PPV_ARGS(m_clearUavHeap.GetAddressOf())
    ));

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
    uavDesc.Texture2D.MipSlice = 0;
    device->CreateUnorderedAccessView(
        m_shadowOutputResource.Get(),
        nullptr,
        &uavDesc,
        m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
    );

    return true;
}
