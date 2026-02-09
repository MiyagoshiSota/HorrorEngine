#include "AccelerationStructure.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/Model/Model.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <cstdio>

// ヘルパー関数: デフォルトヒーププロパティを作成
static D3D12_HEAP_PROPERTIES GetDefaultHeapProperties()
{
    D3D12_HEAP_PROPERTIES props = {};
    props.Type = D3D12_HEAP_TYPE_DEFAULT;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    props.CreationNodeMask = 1;
    props.VisibleNodeMask = 1;
    return props;
}

// ヘルパー関数: アップロードヒーププロパティを作成
static D3D12_HEAP_PROPERTIES GetUploadHeapProperties()
{
    D3D12_HEAP_PROPERTIES props = {};
    props.Type = D3D12_HEAP_TYPE_UPLOAD;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    props.CreationNodeMask = 1;
    props.VisibleNodeMask = 1;
    return props;
}

// ============================================================================
// BottomLevelAS Implementation
// ============================================================================

bool BottomLevelAS::Build(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* commandList,
    const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    if (!device || !commandList || gameObjects.empty())
    {
        return false;
    }

    // ジオメトリ記述子の配列を構築
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs;

    for (const auto& gameObject : gameObjects)
    {
        if (!gameObject)
        {
            continue;
        }

        // MeshRendererコンポーネントからメッシュを取得
        auto meshRenderer = gameObject->FindComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->model)
        {
            continue;
        }

        if (meshRenderer->model->m_Meshes.empty())
        {
            continue;
        }

        // モデルの全サブメッシュをBLASに追加
        for (size_t meshIndex = 0; meshIndex < meshRenderer->model->m_Meshes.size(); ++meshIndex)
        {
            auto mesh = meshRenderer->model->m_Meshes[meshIndex];
            auto vertexBuffer = mesh->get_vertex_buffer();
            auto indexBuffer = mesh->get_index_buffer();

            if (!vertexBuffer || !indexBuffer)
            {
                continue;
            }

            D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
            geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            // 頂点バッファ設定
            geometryDesc.Triangles.VertexBuffer.StartAddress =
                vertexBuffer->GetResource()->GetGPUVirtualAddress();
            geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexBuffer->GetStride();
            geometryDesc.Triangles.VertexCount = vertexBuffer->GetVertexCount();
            geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

            // インデックスバッファ設定
            geometryDesc.Triangles.IndexBuffer =
                indexBuffer->GetResource()->GetGPUVirtualAddress();
            geometryDesc.Triangles.IndexCount = indexBuffer->GetIndexCount();
            geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

            geometryDesc.Triangles.Transform3x4 = 0; // ワールド変換はTLASで適用

            geometryDescs.push_back(geometryDesc);
        }
    }

    if (geometryDescs.empty())
    {
        return false;
    }

    // BLAS構築情報を設定
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.NumDescs = static_cast<UINT>(geometryDescs.size());
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.pGeometryDescs = geometryDescs.data();

    // 必要なバッファサイズを取得
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    // スクラッチバッファの作成（256バイトアライメントが必要）
    UINT64 scratchSize = (prebuildInfo.ScratchDataSizeInBytes + 255) & ~255;
    auto scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
        scratchSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    auto heapProps = GetDefaultHeapProperties();
    
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &scratchDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_scratchBuffer.GetAddressOf())
    ));

    // BLASバッファの作成（256バイトアライメントが必要）
    UINT64 blasSize = (prebuildInfo.ResultDataMaxSizeInBytes + 255) & ~255;
    auto blasDesc = CD3DX12_RESOURCE_DESC::Buffer(
        blasSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &blasDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(m_blasBuffer.GetAddressOf())
    ));

    // BLAS構築記述子を設定
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = m_blasBuffer->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();

    // BLASを構築
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // UAVバリアを挿入（構築完了を待つ）
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_blasBuffer.Get();
    commandList->ResourceBarrier(1, &barrier);

    return true;
}

bool BottomLevelAS::BuildFromBuffers(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* commandList,
    ID3D12Resource* vertexBuffer,
    UINT vertexCount,
    UINT vertexStride,
    ID3D12Resource* indexBuffer,
    UINT indexCount)
{
    if (!device || !commandList || !vertexBuffer || !indexBuffer)
    {
        return false;
    }

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
    geometryDesc.Triangles.VertexCount = vertexCount;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

    geometryDesc.Triangles.IndexBuffer = indexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = indexCount;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

    geometryDesc.Triangles.Transform3x4 = 0;

    // BLAS構築情報を設定
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.NumDescs = 1;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.pGeometryDescs = &geometryDesc;

    // 必要なバッファサイズを取得
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    // スクラッチバッファの作成
    UINT64 scratchSize = (prebuildInfo.ScratchDataSizeInBytes + 255) & ~255;
    auto scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
        scratchSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    auto heapProps = GetDefaultHeapProperties();
    
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &scratchDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_scratchBuffer.GetAddressOf())
    ));

    // BLASバッファの作成
    UINT64 blasSize = (prebuildInfo.ResultDataMaxSizeInBytes + 255) & ~255;
    auto blasDesc = CD3DX12_RESOURCE_DESC::Buffer(
        blasSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &blasDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(m_blasBuffer.GetAddressOf())
    ));

    // BLAS構築
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = m_blasBuffer->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();

    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // UAVバリア
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_blasBuffer.Get();
    commandList->ResourceBarrier(1, &barrier);

    return true;
}

// ============================================================================
// TopLevelAS Implementation
// ============================================================================

bool TopLevelAS::Build(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* commandList,
    const std::vector<Instance>& instances)
{
    if (!device || !commandList || instances.empty())
    {
        return false;
    }

    // インスタンス記述子の配列を作成
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    instanceDescs.reserve(instances.size());

    for (const auto& instance : instances)
    {
        D3D12_RAYTRACING_INSTANCE_DESC desc = {};

        // 3x4変換行列を設定（D3D12 は行優先で instance-to-world を期待。DirectXMath は行優先なので上3行をそのまま渡す）
        DirectX::XMFLOAT3X4 transform3x4;
        DirectX::XMStoreFloat3x4(&transform3x4, instance.transform);
        memcpy(desc.Transform, &transform3x4, sizeof(desc.Transform));

        desc.InstanceID = instance.instanceID;
        desc.InstanceMask = instance.instanceMask;
        desc.InstanceContributionToHitGroupIndex = instance.contributionToHitGroupIndex;
        desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        desc.AccelerationStructure = instance.blasAddress;

        instanceDescs.push_back(desc);
    }

    // インスタンス記述バッファを作成（アップロードヒープ）
    UINT64 instanceDescSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size();
    auto instanceDescBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceDescSize);
    auto uploadHeapProps = GetUploadHeapProperties();

    ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &instanceDescBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_instanceDescBuffer.GetAddressOf())
    ));

    // インスタンス記述データをアップロード
    void* mappedData = nullptr;
    ThrowIfFailed(m_instanceDescBuffer->Map(0, nullptr, &mappedData));
    memcpy(mappedData, instanceDescs.data(), instanceDescSize);
    m_instanceDescBuffer->Unmap(0, nullptr);

    // TLAS構築情報を設定
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    inputs.NumDescs = static_cast<UINT>(instanceDescs.size());
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.InstanceDescs = m_instanceDescBuffer->GetGPUVirtualAddress();

    // 必要なバッファサイズを取得
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

    // スクラッチバッファの作成
    UINT64 scratchSize = (prebuildInfo.ScratchDataSizeInBytes + 255) & ~255;
    auto scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
        scratchSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    auto heapProps = GetDefaultHeapProperties();
    
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &scratchDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(m_scratchBuffer.GetAddressOf())
    ));

    // TLASバッファの作成
    UINT64 tlasSize = (prebuildInfo.ResultDataMaxSizeInBytes + 255) & ~255;
    auto tlasDesc = CD3DX12_RESOURCE_DESC::Buffer(
        tlasSize,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );
    
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &tlasDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(m_tlasBuffer.GetAddressOf())
    ));

    // TLAS構築
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = inputs;
    buildDesc.DestAccelerationStructureData = m_tlasBuffer->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();

    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // UAVバリア
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = m_tlasBuffer.Get();
    commandList->ResourceBarrier(1, &barrier);

    return true;
}

// ============================================================================
// AccelerationStructureManager Implementation
// ============================================================================

bool AccelerationStructureManager::BuildAccelerationStructures(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* commandList,
    const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    if (!device || !commandList || gameObjects.empty())
    {
        return false;
    }

    // 既存のBLASとジオメトリバッファ一覧をクリア
    m_blasList.clear();
    m_geometryBuffers.clear();
    m_geometryCountPerInstance.clear();

    // 各ゲームオブジェクトごとにBLASを作成
    std::vector<TopLevelAS::Instance> tlasInstances;
    UINT accumulatedHitGroupOffset = 0;

    for (size_t i = 0; i < gameObjects.size(); ++i)
    {
        const auto& gameObject = gameObjects[i];
        if (!gameObject)
        {
            continue;
        }

        auto meshRenderer = gameObject->FindComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->model || meshRenderer->model->m_Meshes.empty())
        {
            continue;
        }

        const size_t meshCount = meshRenderer->model->m_Meshes.size();
        for (size_t m = 0; m < meshCount; ++m)
        {
            auto mesh = meshRenderer->model->m_Meshes[m];
            auto vb = mesh->get_vertex_buffer();
            auto ib = mesh->get_index_buffer();
            if (vb && ib && vb->GetResource() && ib->GetResource())
            {
                GeometryBuffers gb;
                gb.vertexBuffer = vb->GetResource();
                gb.indexBuffer = ib->GetResource();
                m_geometryBuffers.push_back(gb);
            }
        }
        m_geometryCountPerInstance.push_back(static_cast<UINT>(meshCount));

        auto blas = std::make_unique<BottomLevelAS>();
        std::vector<std::shared_ptr<GameObject>> singleObject = { gameObject };
        if (!blas->Build(device, commandList, singleObject))
        {
            m_geometryBuffers.resize(m_geometryBuffers.size() - meshCount);
            m_geometryCountPerInstance.pop_back();
            continue;
        }

        TopLevelAS::Instance instance;
        instance.transform = gameObject->GetTransform();
        instance.instanceID = static_cast<UINT>(i);
        instance.instanceMask = 0xFF;
        instance.contributionToHitGroupIndex = accumulatedHitGroupOffset;
        instance.blasAddress = blas->GetGpuAddress();

        accumulatedHitGroupOffset += static_cast<UINT>(meshCount);
        tlasInstances.push_back(instance);
        m_blasList.push_back(std::move(blas));
        printf("[AccelerationStructure] BLAS[%zu] ジオメトリ数 = %zu\n", m_blasList.size(), meshCount);
    }

    if (tlasInstances.empty())
    {
        return false;
    }

    // TLASを作成
    m_tlas = std::make_unique<TopLevelAS>();
    if (!m_tlas->Build(device, commandList, tlasInstances))
    {
        return false;
    }

    return true;
}

bool AccelerationStructureManager::UpdateTopLevelAS(
    ID3D12Device5* device,
    ID3D12GraphicsCommandList4* commandList,
    const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    if (!device || !commandList || gameObjects.empty() || !m_tlas)
    {
        return false;
    }

    std::vector<TopLevelAS::Instance> tlasInstances;
    UINT accumulatedHitGroupOffset = 0;

    for (size_t i = 0; i < gameObjects.size() && i < m_blasList.size(); ++i)
    {
        const auto& gameObject = gameObjects[i];
        if (!gameObject)
        {
            continue;
        }
        auto meshRenderer = gameObject->FindComponent<MeshRenderer>();
        if (!meshRenderer || !meshRenderer->model || meshRenderer->model->m_Meshes.empty())
        {
            continue;
        }
        UINT geomCount = (i < m_geometryCountPerInstance.size()) ? m_geometryCountPerInstance[i] : 0;

        TopLevelAS::Instance instance;
        instance.transform = gameObject->GetTransform();
        instance.instanceID = static_cast<UINT>(i);
        instance.instanceMask = 0xFF;
        instance.contributionToHitGroupIndex = accumulatedHitGroupOffset;
        instance.blasAddress = m_blasList[i]->GetGpuAddress();

        accumulatedHitGroupOffset += geomCount;
        tlasInstances.push_back(instance);
    }

    // TLASを再構築
    return m_tlas->Build(device, commandList, tlasInstances);
}
