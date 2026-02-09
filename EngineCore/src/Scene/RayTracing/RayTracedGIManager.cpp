#include "RayTracedGIManager.h"
#include "Renderer/Engine.h"
#include "Modules/DxHelper.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Model/Model.h"
#include "Scene/GameObject/Material/Material.h"
#include "Renderer/Texture/TextureResourceManager.h"
#include "Renderer/Texture/Texture2D.h"
#include <d3dx12.h>
#include <Windows.h>
#include <stdio.h>

bool RayTracedGIManager::Init(ID3D12Device5* device, UINT width, UINT height)
{
    if (!device)
    {
        printf("[RayTracedGIManager] エラー: デバイスがnullです\n");
        return false;
    }

    m_width = width;
    m_height = height;

    m_pipelineState = std::make_unique<RayTracingPipelineState>();
    const std::wstring shaderPath = L"../EngineCore/src/Renderer/RayTracing/Shaders/RTGIRayTracing.hlsl";

    if (!m_pipelineState->CreateForRTGI(device, shaderPath.c_str(), 12, 8))
    {
        printf("[RayTracedGIManager] エラー: RTGI Pipeline Stateの作成に失敗しました\n");
        return false;
    }

    m_shaderBindingTable = std::make_unique<ShaderBindingTable>();
    if (!m_shaderBindingTable->BuildForRTGI(device, m_pipelineState.get()))
    {
        printf("[RayTracedGIManager] エラー: RTGI SBTの作成に失敗しました\n");
        return false;
    }

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
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
        IID_PPV_ARGS(m_giOutputResource.GetAddressOf())
    ));
    m_giOutputResource->SetName(L"RTGIOutput");
    m_giOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    auto srvHandle = g_Engine->GetDescriptorHeap()->Allocate(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    device->CreateShaderResourceView(m_giOutputResource.Get(), &srvDesc, srvHandle->cpuHandle);

    m_giTarget = std::make_shared<RayTracedGITarget>();
    m_giTarget->SetResource(m_giOutputResource);
    m_giTarget->SetSrv(srvHandle);

    m_rtgiDescriptors = g_Engine->GetDescriptorHeap()->Allocate(2);
    if (!m_rtgiDescriptors)
    {
        printf("[RayTracedGIManager] エラー: エンジンヒープの Allocate(2) に失敗しました\n");
        return false;
    }
    const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE giUavCpu = m_rtgiDescriptors->cpuHandle;
    giUavCpu.ptr += descriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.Texture2D.MipSlice = 0;
    device->CreateUnorderedAccessView(m_giOutputResource.Get(), nullptr, &uavDesc, giUavCpu);

    if (!CreateClearUavHeap(device))
    {
        printf("[RayTracedGIManager] エラー: ClearUAV用ヒープの作成に失敗しました\n");
        return false;
    }

    const UINT cbSize = (sizeof(RayTracedGIConstants) + 255) & ~255;
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
    printf("[RayTracedGIManager] RTGI Manager 初期化成功\n");
    return true;
}

RayTracedGIRenderData RayTracedGIManager::GetRenderData(UINT frameIndex, AccelerationStructureManager* asManager) const
{
    RayTracedGIRenderData data = {};
    if (!m_initialized || !asManager || !m_rtgiDescriptors)
        return data;
    const UINT index = frameIndex % kFrameBufferCount;
    const UINT descriptorSize = g_Engine->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    data.asManager = asManager;
    data.pipelineState = m_pipelineState.get();
    data.shaderBindingTable = m_shaderBindingTable.get();
    data.giOutputResource = m_giOutputResource.Get();
    data.descriptorHeap = g_Engine->GetDescriptorHeap()->GetHeap();
    data.tlasSrvCpuHandle = m_rtgiDescriptors->cpuHandle;
    data.tlasSrvGpuHandle = m_rtgiDescriptors->gpuHandle;
    data.giUavGpuHandle = m_rtgiDescriptors->gpuHandle;
    data.giUavGpuHandle.ptr += descriptorSize;
    data.clearUavCpuHandle = m_clearUavHeap
        ? m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
        : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    data.descriptorIncrementSize = descriptorSize;
    data.constantBuffer = m_constantBuffers[index].Get();
    data.pGiOutputState = const_cast<D3D12_RESOURCE_STATES*>(&m_giOutputState);
    data.width = m_width;
    data.height = m_height;
    data.giTarget = m_giTarget;
    data.giManager = const_cast<RayTracedGIManager*>(this);
    data.isValid = true;
    return data;
}

void RayTracedGIManager::UpdateConstants(const RayTracedGIConstants& constants, UINT frameIndex)
{
    const UINT index = frameIndex % kFrameBufferCount;
    ID3D12Resource* cb = m_constantBuffers[index].Get();
    if (!cb) return;
    void* mapped = nullptr;
    if (SUCCEEDED(cb->Map(0, nullptr, &mapped)))
    {
        memcpy(mapped, &constants, sizeof(RayTracedGIConstants));
        cb->Unmap(0, nullptr);
    }
}

void RayTracedGIManager::SetGiOutputState(D3D12_RESOURCE_STATES state)
{
    m_giOutputState = state;
}

void RayTracedGIManager::CreateByteAddressSRV(ID3D12Device5* device, ID3D12Resource* buffer, UINT sizeInBytes, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle)
{
    if (!device || !buffer || sizeInBytes == 0) return;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = (sizeInBytes + 3u) / 4u;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    device->CreateShaderResourceView(buffer, &srvDesc, destCpuHandle);
}

void RayTracedGIManager::EnsureGeometryDescriptorsAndSBT(
    ID3D12Device5* device,
    const AccelerationStructureManager* asManager,
    const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    if (!device || !asManager || !asManager->IsBuilt() || !m_pipelineState || !m_shaderBindingTable)
        return;
    const UINT N = asManager->GetTotalGeometryCount();
    if (N == 0) return;
    if (N == m_cachedGeometryCount && m_geometryVBIbDescriptors)
        return;

    m_cachedGeometryCount = N;
    m_geometryVBIbDescriptors = g_Engine->GetDescriptorHeap()->Allocate(3u * N);
    if (!m_geometryVBIbDescriptors)
    {
        m_cachedGeometryCount = 0;
        return;
    }
    const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const auto& geometryBuffers = asManager->GetGeometryBuffers();

    const auto whiteTex = TextureResourceManager::Instance().WhiteTexture();
    const D3D12_SHADER_RESOURCE_VIEW_DESC* whiteViewDesc = whiteTex ? &whiteTex->GetViewDesc() : nullptr;
    ID3D12Resource* whiteResource = whiteTex ? whiteTex->GetResource() : nullptr;

    UINT geometryIndex = 0;
    for (const auto& gameObject : gameObjects)
    {
        if (!gameObject) continue;
        const auto meshRenderer = gameObject->FindComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->model || meshRenderer->model->m_Meshes.empty())
            continue;
        const auto* model = meshRenderer->model.get();
        const size_t meshCount = model->m_Meshes.size();
        for (size_t m = 0; m < meshCount && geometryIndex < N; ++m)
        {
            if (geometryIndex >= geometryBuffers.size())
                break;
            const auto& gb = geometryBuffers[geometryIndex];
            if (!gb.vertexBuffer || !gb.indexBuffer)
            {
                ++geometryIndex;
                continue;
            }
            D3D12_CPU_DESCRIPTOR_HANDLE cpuVb = m_geometryVBIbDescriptors->cpuHandle;
            cpuVb.ptr += (3u * geometryIndex) * descriptorSize;
            D3D12_CPU_DESCRIPTOR_HANDLE cpuIb = m_geometryVBIbDescriptors->cpuHandle;
            cpuIb.ptr += (3u * geometryIndex + 1u) * descriptorSize;
            D3D12_CPU_DESCRIPTOR_HANDLE cpuAlbedo = m_geometryVBIbDescriptors->cpuHandle;
            cpuAlbedo.ptr += (3u * geometryIndex + 2u) * descriptorSize;

            D3D12_RESOURCE_DESC vbDesc = gb.vertexBuffer->GetDesc();
            D3D12_RESOURCE_DESC ibDesc = gb.indexBuffer->GetDesc();
            CreateByteAddressSRV(device, gb.vertexBuffer, static_cast<UINT>(vbDesc.Width), cpuVb);
            CreateByteAddressSRV(device, gb.indexBuffer, static_cast<UINT>(ibDesc.Width), cpuIb);

            if (m < model->m_Materials.size())
            {
                const auto& material = model->m_Materials[m];
                const auto albedoTexture = material ? material->GetTexture("_MainTex") : nullptr;
                if (albedoTexture && albedoTexture->GetResource())
                {
                    const auto& viewDesc = albedoTexture->GetViewDesc();
                    device->CreateShaderResourceView(albedoTexture->GetResource(), &viewDesc, cpuAlbedo);
                }
                else if (whiteResource && whiteViewDesc)
                    device->CreateShaderResourceView(whiteResource, whiteViewDesc, cpuAlbedo);
            }
            else if (whiteResource && whiteViewDesc)
                device->CreateShaderResourceView(whiteResource, whiteViewDesc, cpuAlbedo);

            ++geometryIndex;
        }
    }

    if (!m_shaderBindingTable->BuildForRTGI(device, m_pipelineState.get(), N, m_geometryVBIbDescriptors->gpuHandle, descriptorSize))
        m_cachedGeometryCount = 0;
}

bool RayTracedGIManager::CreateClearUavHeap(ID3D12Device5* device)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_clearUavHeap.GetAddressOf())));
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.Texture2D.MipSlice = 0;
    device->CreateUnorderedAccessView(
        m_giOutputResource.Get(),
        nullptr,
        &uavDesc,
        m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
    );
    return true;
}
