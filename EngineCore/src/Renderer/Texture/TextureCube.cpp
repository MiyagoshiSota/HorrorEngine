#include "TextureCube.h"
#include <DirectXTex.h>
#include <vector>
#include <cassert>
#include <d3dx12.h>
#include <Windows.h>

#include "Renderer/Engine.h"
#include "Modules/DxHelper.h"

#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

TextureCube::TextureCube()
    : m_size(0)
{
}

TextureCube::~TextureCube()
{
}

std::shared_ptr<TextureCube> TextureCube::Load(const std::wstring& path)
{
    auto tex = std::make_shared<TextureCube>();

    if (!tex->InternalLoad(path))
    {
        return nullptr;
    }
    return tex;
}

bool TextureCube::InternalLoad(const std::wstring& path)
{
    m_path = path;

    // DDSファイルからロード
    TexMetadata meta = {};
    ScratchImage scratch = {};

    HRESULT hr = LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, &meta, scratch);

    try
    {
        ThrowIfFailed(hr);
    }
    catch (const std::exception&)
    {
        return false;
    }

    // キューブマップであることを確認
    if (!meta.IsCubemap())
    {
        return false;
    }

    m_size = static_cast<uint32_t>(meta.width);

    // GPUリソース作成（DEFAULT ヒープ、COPY_DEST状態）
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        meta.format,
        meta.width,
        static_cast<UINT>(meta.height),
        static_cast<UINT16>(meta.arraySize), // キューブマップは6面
        static_cast<UINT16>(meta.mipLevels)
    );
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
            &defaultHeapProp,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_resource)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    // サブリソースの数を計算（6面 × ミップレベル数）
    const UINT numSubresources = static_cast<UINT>(meta.arraySize * meta.mipLevels);

    // アップロードバッファサイズを計算
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
    std::vector<UINT> numRows(numSubresources);
    std::vector<UINT64> rowSizeInBytes(numSubresources);
    UINT64 uploadBufferSize = 0;

    g_Engine->Device()->GetCopyableFootprints(
        &resDesc,
        0,
        numSubresources,
        0,
        layouts.data(),
        numRows.data(),
        rowSizeInBytes.data(),
        &uploadBufferSize
    );

    // ステージングバッファ作成（UPLOAD ヒープ）
    auto uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
            &uploadHeapProp,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    // CPUからステージングバッファにデータコピー
    uint8_t* pData = nullptr;
    try
    {
        D3D12_RANGE readRange = { 0, 0 };
        ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));

        for (UINT i = 0; i < numSubresources; ++i)
        {
            // サブリソースのインデックスからミップレベルと配列インデックスを取得
            const UINT arrayIndex = i / static_cast<UINT>(meta.mipLevels);
            const UINT mipLevel = i % static_cast<UINT>(meta.mipLevels);

            const Image* img = scratch.GetImage(mipLevel, arrayIndex, 0);
            if (!img)
            {
                uploadBuffer->Unmap(0, nullptr);
                return false;
            }

            const uint8_t* pSrcData = img->pixels;
            const UINT srcRowPitch = static_cast<UINT>(img->rowPitch);
            const UINT rowSize = static_cast<UINT>(rowSizeInBytes[i]);

            for (UINT row = 0; row < numRows[i]; ++row)
            {
                memcpy(pData + layouts[i].Offset + row * layouts[i].Footprint.RowPitch,
                       pSrcData + row * srcRowPitch,
                       rowSize);
            }
        }

        D3D12_RANGE writeRange = { 0, static_cast<SIZE_T>(uploadBufferSize) };
        uploadBuffer->Unmap(0, &writeRange);
    }
    catch (const std::exception&)
    {
        return false;
    }

    // コマンドリスト作成
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    UINT64 fenceValue = 1;

    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator)
        ));

        ThrowIfFailed(g_Engine->Device()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList)
        ));

        ThrowIfFailed(g_Engine->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    }
    catch (const std::exception&)
    {
        if (fenceEvent) CloseHandle(fenceEvent);
        return false;
    }

    // GPUコピーコマンド発行
    try
    {
        for (UINT i = 0; i < numSubresources; ++i)
        {
            CD3DX12_TEXTURE_COPY_LOCATION dst(m_resource.Get(), i);
            CD3DX12_TEXTURE_COPY_LOCATION src(uploadBuffer.Get(), layouts[i]);
            commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        commandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(commandList->Close());

        ID3D12CommandList* cmdLists[] = { commandList.Get() };
        g_Engine->CommandQueue()->ExecuteCommandLists(1, cmdLists);

        // GPU完了待機
        ThrowIfFailed(g_Engine->CommandQueue()->Signal(fence.Get(), fenceValue));
        if (fence->GetCompletedValue() < fenceValue)
        {
            ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }
    catch (const std::exception&)
    {
        if (fenceEvent) CloseHandle(fenceEvent);
        return false;
    }

    CloseHandle(fenceEvent);

    // SRV設定（キューブマップ用）
    m_srvDesc.Format = meta.format;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    m_srvDesc.TextureCube.MostDetailedMip = 0;
    m_srvDesc.TextureCube.MipLevels = static_cast<UINT>(meta.mipLevels);
    m_srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    return true;
}
