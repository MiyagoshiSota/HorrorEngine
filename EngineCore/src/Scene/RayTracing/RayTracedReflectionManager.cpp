#include "RayTracedReflectionManager.h"
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
#include <algorithm>
#include <cmath>

bool RayTracedReflectionManager::Init(ID3D12Device5* device, UINT width, UINT height)
{
    if (!device)
    {
        printf("[RayTracedReflectionManager] エラー: デバイスがnullです\n");
        return false;
    }

    m_width = width;
    m_height = height;

    m_pipelineState = std::make_unique<RayTracingPipelineState>();
    const std::wstring shaderPath = L"../EngineCore/src/Renderer/RayTracing/Shaders/RTReflection.hlsl";

    if (!m_pipelineState->CreateForRTReflection(device, shaderPath.c_str(), 24, 8))
    {
        printf("[RayTracedReflectionManager] エラー: RT Reflection Pipeline Stateの作成に失敗しました\n");
        return false;
    }

    m_shaderBindingTable = std::make_unique<ShaderBindingTable>();
    if (!m_shaderBindingTable->BuildForRTReflection(device, m_pipelineState.get()))
    {
        printf("[RayTracedReflectionManager] エラー: RT Reflection SBTの作成に失敗しました\n");
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
        IID_PPV_ARGS(m_reflectionOutputResource.GetAddressOf())
    ));
    m_reflectionOutputResource->SetName(L"RTReflectionOutput");
    m_reflectionOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    auto srvHandle = g_Engine->GetDescriptorHeap()->Allocate(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    device->CreateShaderResourceView(m_reflectionOutputResource.Get(), &srvDesc, srvHandle->cpuHandle);

    m_reflectionTarget = std::make_shared<RayTracedReflectionTarget>();
    m_reflectionTarget->SetResource(m_reflectionOutputResource);
    m_reflectionTarget->SetSrv(srvHandle);

    m_rtReflectionDescriptors = g_Engine->GetDescriptorHeap()->Allocate(2);
    if (!m_rtReflectionDescriptors)
    {
        printf("[RayTracedReflectionManager] エラー: エンジンヒープの Allocate(2) に失敗しました\n");
        return false;
    }
    const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE reflectionUavCpu = m_rtReflectionDescriptors->cpuHandle;
    reflectionUavCpu.ptr += descriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.Texture2D.MipSlice = 0;
    device->CreateUnorderedAccessView(m_reflectionOutputResource.Get(), nullptr, &uavDesc, reflectionUavCpu);

    if (!CreateClearUavHeap(device))
    {
        printf("[RayTracedReflectionManager] エラー: ClearUAV用ヒープの作成に失敗しました\n");
        return false;
    }

    const UINT cbSize = (sizeof(RayTracedReflectionConstants) + 255) & ~255;
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
    printf("[RayTracedReflectionManager] RT Reflection Manager 初期化成功\n");
    return true;
}

RayTracedReflectionRenderData RayTracedReflectionManager::GetRenderData(UINT frameIndex, AccelerationStructureManager* asManager) const
{
    RayTracedReflectionRenderData data = {};
    if (!m_initialized || !asManager || !m_rtReflectionDescriptors)
        return data;
    const UINT index = frameIndex % kFrameBufferCount;
    const UINT descriptorSize = g_Engine->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    data.asManager = asManager;
    data.pipelineState = m_pipelineState.get();
    data.shaderBindingTable = m_shaderBindingTable.get();
    data.reflectionOutputResource = m_reflectionOutputResource.Get();
    data.descriptorHeap = g_Engine->GetDescriptorHeap()->GetHeap();
    data.tlasSrvCpuHandle = m_rtReflectionDescriptors->cpuHandle;
    data.tlasSrvGpuHandle = m_rtReflectionDescriptors->gpuHandle;
    data.reflectionUavGpuHandle = m_rtReflectionDescriptors->gpuHandle;
    data.reflectionUavGpuHandle.ptr += descriptorSize;
    data.clearUavCpuHandle = m_clearUavHeap
        ? m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
        : D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    data.descriptorIncrementSize = descriptorSize;
    data.constantBuffer = m_constantBuffers[index].Get();
    data.pReflectionOutputState = const_cast<D3D12_RESOURCE_STATES*>(&m_reflectionOutputState);
    data.width = m_width;
    data.height = m_height;
    data.reflectionTarget = m_reflectionTarget;
    data.reflectionManager = const_cast<RayTracedReflectionManager*>(this);
    data.isValid = true;
    return data;
}

void RayTracedReflectionManager::UpdateConstants(const RayTracedReflectionConstants& constants, UINT frameIndex)
{
    const UINT index = frameIndex % kFrameBufferCount;
    ID3D12Resource* cb = m_constantBuffers[index].Get();
    if (!cb) return;
    void* mapped = nullptr;
    if (SUCCEEDED(cb->Map(0, nullptr, &mapped)))
    {
        memcpy(mapped, &constants, sizeof(RayTracedReflectionConstants));
        cb->Unmap(0, nullptr);
    }
}

void RayTracedReflectionManager::SetReflectionOutputState(D3D12_RESOURCE_STATES state)
{
    m_reflectionOutputState = state;
}

void RayTracedReflectionManager::CreateByteAddressSRV(ID3D12Device5* device, ID3D12Resource* buffer, UINT sizeInBytes, D3D12_CPU_DESCRIPTOR_HANDLE destCpuHandle)
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

void RayTracedReflectionManager::EnsureGeometryDescriptorsAndSBT(
    ID3D12Device5* device,
    const AccelerationStructureManager* asManager,
    const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    if (!device || !asManager || !asManager->IsBuilt() || !m_pipelineState || !m_shaderBindingTable)
        return;
    const UINT N = asManager->GetTotalGeometryCount();
    if (N == 0) return;
    if (N == m_cachedGeometryCount && m_geometryVBIbDescriptors && !m_debugGeometryColors)
        return;

    if (N != m_cachedGeometryCount || !m_geometryVBIbDescriptors)
    {
        m_cachedGeometryCount = N;
        m_geometryVBIbDescriptors = g_Engine->GetDescriptorHeap()->Allocate(3u * N);
        if (!m_geometryVBIbDescriptors)
        {
            m_cachedGeometryCount = 0;
            return;
        }
    }
    const UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const auto& geometryBuffers = asManager->GetGeometryBuffers();

    // 並び比較用: デスクリプタ／SBT 構築時のジオメトリ順をログ（AS 構築時と一致するか調査）
    printf("[RTReflection/SBT] EnsureGeometryDescriptorsAndSBT: geometryCount=%u (asManager buffers=%zu)\n",
        N, geometryBuffers.size());
    ID3D12DescriptorHeap* engineHeap = g_Engine->GetDescriptorHeap()->GetHeap();
    printf("[RTReflection/Heap] geometry descriptors from engine heap=%p\n", engineHeap);

    const auto whiteTex = TextureResourceManager::Instance().WhiteTexture();
    const D3D12_SHADER_RESOURCE_VIEW_DESC* whiteViewDesc = whiteTex ? &whiteTex->GetViewDesc() : nullptr;
    ID3D12Resource* whiteResource = whiteTex ? whiteTex->GetResource() : nullptr;

    if (m_debugGeometryColors)
    {
        if (m_debugColorTextures.size() < N)
            m_debugColorTextures.resize(N);
        const D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
        const D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
        bool createdAny = false;
        for (UINT k = 0; k < N; ++k)
        {
            if (m_debugColorTextures[k])
                continue;
            createdAny = true;
            ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_PPV_ARGS(m_debugColorTextures[k].GetAddressOf())));
            const float t = static_cast<float>(k) / static_cast<float>(std::max(1u, N));
            uint8_t r = static_cast<uint8_t>(255.0f * (0.5f + 0.5f * sinf(t * 6.28f)));
            uint8_t g = static_cast<uint8_t>(255.0f * (0.5f + 0.5f * sinf(t * 6.28f + 2.0f)));
            uint8_t b = static_cast<uint8_t>(255.0f * (0.5f + 0.5f * sinf(t * 6.28f + 4.0f)));
            uint32_t pixel = (255u << 24) | (r << 16) | (g << 8) | b;
            m_debugColorTextures[k]->WriteToSubresource(0, nullptr, &pixel, 4, 4);
        }
        if (createdAny)
            printf("[RTReflection/DEBUG] Created debug color textures for %u geometries (distinct color per geometry)\n", N);
    }

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
            if (geometryIndex < 32u)
                printf("  [SBT] geom#%u vb=%p go=%p\n", geometryIndex, gb.vertexBuffer, gameObject.get());
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

            if (m_debugGeometryColors && geometryIndex < m_debugColorTextures.size() && m_debugColorTextures[geometryIndex])
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;
                device->CreateShaderResourceView(m_debugColorTextures[geometryIndex].Get(), &srvDesc, cpuAlbedo);
            }
            else if (m < model->m_Materials.size())
            {
                const auto& material = model->m_Materials[m];
                const auto albedoTexture = material ? material->GetTexture("_MainTex") : nullptr;
                if (albedoTexture && albedoTexture->GetResource())
                {
                    const auto& viewDesc = albedoTexture->GetViewDesc();
                    device->CreateShaderResourceView(albedoTexture->GetResource(), &viewDesc, cpuAlbedo);
                    if (geometryIndex < 10u)
                        printf("[RTReflection/Model] geom#%u albedo=_MainTex resource=%p\n",
                            geometryIndex, albedoTexture->GetResource());
                }
                else if (whiteResource && whiteViewDesc)
                {
                    device->CreateShaderResourceView(whiteResource, whiteViewDesc, cpuAlbedo);
                    if (geometryIndex < 10u)
                        printf("[RTReflection/Model] geom#%u albedo=white (no _MainTex)\n", geometryIndex);
                }
            }
            else if (whiteResource && whiteViewDesc)
            {
                device->CreateShaderResourceView(whiteResource, whiteViewDesc, cpuAlbedo);
                if (geometryIndex < 10u)
                    printf("[RTReflection/Model] geom#%u albedo=white (no material)\n", geometryIndex);
            }

            ++geometryIndex;
        }
    }
    if (N > 32u)
        printf("  [SBT] ... and %u more\n", N - 32u);

    if (!m_shaderBindingTable->BuildForRTReflection(device, m_pipelineState.get(), N, m_geometryVBIbDescriptors->gpuHandle, descriptorSize))
        m_cachedGeometryCount = 0;
}

bool RayTracedReflectionManager::CreateClearUavHeap(ID3D12Device5* device)
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
        m_reflectionOutputResource.Get(),
        nullptr,
        &uavDesc,
        m_clearUavHeap->GetCPUDescriptorHandleForHeapStart()
    );
    return true;
}
