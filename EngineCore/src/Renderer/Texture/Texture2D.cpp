#include "Texture2D.h"
#include <DirectXTex.h>
#include <vector>
#include <cassert>
#include <d3dx12.h>
#include <Windows.h>

#include "Renderer/Engine.h"
#include "Modules/DxHelper.h"

#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

std::wstring GetWideString(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::wstring FileExtension(const std::wstring& path)
{
    auto idx = path.rfind(L'.');
    if (idx == std::wstring::npos) return L"";
    return path.substr(idx + 1);
}

Texture2D::Texture2D()
    : m_width(0), m_height(0)
{
}

Texture2D::~Texture2D()
{
}

std::shared_ptr<Texture2D> Texture2D::Load(const std::wstring& path)
{
    auto tex = std::make_shared<Texture2D>();

    if (!tex->InternalLoad(path))
    {
        return nullptr;
    }
    
    return tex;
}

std::shared_ptr<Texture2D> Texture2D::LoadFromMemory(const uint8_t* data, size_t dataSize)
{
    if (data == nullptr || dataSize == 0) return nullptr;
    auto tex = std::make_shared<Texture2D>();
    if (!tex->InternalLoadFromMemory(data, dataSize))
        return nullptr;
    return tex;
}

std::shared_ptr<Texture2D> Texture2D::CreateFromRawRGBA(const uint8_t* data, uint32_t width, uint32_t height)
{
    if (data == nullptr || width == 0 || height == 0) return nullptr;
    auto tex = std::make_shared<Texture2D>();
    const size_t dataSize = static_cast<size_t>(width) * height * 4;
    if (!tex->InternalCreateFromData(data, dataSize, width, height))
        return nullptr;
    return tex;
}

std::shared_ptr<Texture2D> Texture2D::CreateWhiteTexture()
{
    auto tex = std::make_shared<Texture2D>();

    uint32_t whitePixel = 0x00000000;

    bool success = tex->InternalCreateFromData(
        reinterpret_cast<const uint8_t*>(&whitePixel),
        sizeof(uint32_t),
        1,
        1
    );

    if (!success)
    {
        return nullptr;
    }

    return tex;
}

bool Texture2D::InternalLoad(const std::wstring& path)
{
    m_path = path;

    // ファイルI/O + デコード
    TexMetadata meta = {};
    ScratchImage scratch = {};
    std::wstring ext = FileExtension(path);
    HRESULT hr = E_FAIL;

    if (ext == L"tga" || ext == L"TGA")
    {
        hr = LoadFromTGAFile(path.c_str(), &meta, scratch);
    }
    else
    {
        hr = LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &meta, scratch);
    }

    try
    {
        ThrowIfFailed(hr);
    }
    catch (const std::exception&)
    {
        return false;
    }

    m_width = static_cast<uint32_t>(meta.width);
    m_height = static_cast<uint32_t>(meta.height);

    // ミップマップ自動生成
    ScratchImage mipChain;
    if (meta.mipLevels == 1)
    {
        // ミップマップが含まれていない場合は自動生成
        hr = GenerateMipMaps(
            scratch.GetImages(),
            scratch.GetImageCount(),
            scratch.GetMetadata(),
            TEX_FILTER_DEFAULT,
            0, // 自動でミップレベル数を計算
            mipChain
        );

        try
        {
            ThrowIfFailed(hr);
        }
        catch (const std::exception&)
        {
            // ミップマップ生成に失敗した場合は元の画像を使用
            mipChain = std::move(scratch);
        }
    }
    else
    {
        // 既にミップマップが含まれている場合はそのまま使用
        mipChain = std::move(scratch);
    }

    // ミップチェーンのメタデータを取得
    const TexMetadata& mipMeta = mipChain.GetMetadata();

    // GPUリソース作成（DEFAULT ヒープ、COPY_DEST状態）
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        mipMeta.format,
        mipMeta.width,
        mipMeta.height,
        static_cast<UINT16>(mipMeta.arraySize),
        static_cast<UINT16>(mipMeta.mipLevels)
    );

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

    // 全ミップレベル分のレイアウト計算
    const UINT numSubresources = static_cast<UINT>(mipMeta.mipLevels);
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
    std::vector<UINT> numRows(numSubresources);
    std::vector<UINT64> rowSizesInBytes(numSubresources);
    UINT64 uploadBufferSize = 0;

    g_Engine->Device()->GetCopyableFootprints(
        &resDesc,
        0,  // FirstSubresource
        numSubresources,
        0,  // BaseOffset
        layouts.data(),
        numRows.data(),
        rowSizesInBytes.data(),
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

    // 全ミップレベルをステージングバッファにコピー
    uint8_t* pData = nullptr;
    try
    {
        D3D12_RANGE readRange = { 0, 0 };
        ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));

        for (UINT mipLevel = 0; mipLevel < numSubresources; ++mipLevel)
        {
            const Image* mipImage = mipChain.GetImage(mipLevel, 0, 0);
            if (!mipImage)
            {
                continue;
            }

            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layouts[mipLevel];
            const uint8_t* pSrcData = mipImage->pixels;
            const UINT srcRowPitch = static_cast<UINT>(mipImage->rowPitch);
            const UINT dstRowPitch = layout.Footprint.RowPitch;
            const UINT rowSize = static_cast<UINT>(rowSizesInBytes[mipLevel]);

            for (UINT row = 0; row < numRows[mipLevel]; ++row)
            {
                memcpy(pData + layout.Offset + row * dstRowPitch,
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

    // 全ミップレベルのGPUコピーコマンド発行
    try
    {
        for (UINT mipLevel = 0; mipLevel < numSubresources; ++mipLevel)
        {
            CD3DX12_TEXTURE_COPY_LOCATION dst(m_resource.Get(), mipLevel);
            CD3DX12_TEXTURE_COPY_LOCATION src(uploadBuffer.Get(), layouts[mipLevel]);
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

    // SRV設定
    m_srvDesc.Format = resDesc.Format;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    m_srvDesc.Texture2D.MipLevels = static_cast<UINT>(mipMeta.mipLevels);
    m_srvDesc.Texture2D.MostDetailedMip = 0;

    return true;
}

bool Texture2D::InternalLoadFromMemory(const uint8_t* data, size_t dataSize)
{
    TexMetadata meta = {};
    ScratchImage scratch = {};
    HRESULT hr = LoadFromWICMemory(data, dataSize, WIC_FLAGS_NONE, &meta, scratch);
    try
    {
        ThrowIfFailed(hr);
    }
    catch (const std::exception&)
    {
        return false;
    }

    m_width = static_cast<uint32_t>(meta.width);
    m_height = static_cast<uint32_t>(meta.height);

    ScratchImage mipChain;
    if (meta.mipLevels == 1)
    {
        hr = GenerateMipMaps(
            scratch.GetImages(),
            scratch.GetImageCount(),
            scratch.GetMetadata(),
            TEX_FILTER_DEFAULT,
            0,
            mipChain
        );
        try
        {
            ThrowIfFailed(hr);
        }
        catch (const std::exception&)
        {
            mipChain = std::move(scratch);
        }
    }
    else
    {
        mipChain = std::move(scratch);
    }

    const TexMetadata& mipMeta = mipChain.GetMetadata();
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        mipMeta.format,
        mipMeta.width,
        mipMeta.height,
        static_cast<UINT16>(mipMeta.arraySize),
        static_cast<UINT16>(mipMeta.mipLevels)
    );

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

    const UINT numSubresources = static_cast<UINT>(mipMeta.mipLevels);
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(numSubresources);
    std::vector<UINT> numRows(numSubresources);
    std::vector<UINT64> rowSizesInBytes(numSubresources);
    UINT64 uploadBufferSize = 0;
    g_Engine->Device()->GetCopyableFootprints(
        &resDesc, 0, numSubresources, 0,
        layouts.data(), numRows.data(), rowSizesInBytes.data(), &uploadBufferSize
    );

    auto uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
            &uploadHeapProp, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    uint8_t* pData = nullptr;
    try
    {
        D3D12_RANGE readRange = { 0, 0 };
        ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
        for (UINT mipLevel = 0; mipLevel < numSubresources; ++mipLevel)
        {
            const Image* mipImage = mipChain.GetImage(mipLevel, 0, 0);
            if (!mipImage) continue;
            const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = layouts[mipLevel];
            const uint8_t* pSrcData = mipImage->pixels;
            const UINT srcRowPitch = static_cast<UINT>(mipImage->rowPitch);
            const UINT dstRowPitch = layout.Footprint.RowPitch;
            const UINT rowSize = static_cast<UINT>(rowSizesInBytes[mipLevel]);
            for (UINT row = 0; row < numRows[mipLevel]; ++row)
            {
                memcpy(pData + layout.Offset + row * dstRowPitch,
                       pSrcData + row * srcRowPitch, rowSize);
            }
        }
        D3D12_RANGE writeRange = { 0, static_cast<SIZE_T>(uploadBufferSize) };
        uploadBuffer->Unmap(0, &writeRange);
    }
    catch (const std::exception&)
    {
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    UINT64 fenceValue = 1;
    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
        ThrowIfFailed(g_Engine->Device()->CreateCommandList(
            0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
            IID_PPV_ARGS(&commandList)));
        ThrowIfFailed(g_Engine->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    }
    catch (const std::exception&)
    {
        if (fenceEvent) CloseHandle(fenceEvent);
        return false;
    }
    try
    {
        for (UINT mipLevel = 0; mipLevel < numSubresources; ++mipLevel)
        {
            CD3DX12_TEXTURE_COPY_LOCATION dst(m_resource.Get(), mipLevel);
            CD3DX12_TEXTURE_COPY_LOCATION src(uploadBuffer.Get(), layouts[mipLevel]);
            commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);
        ThrowIfFailed(commandList->Close());
        ID3D12CommandList* cmdLists[] = { commandList.Get() };
        g_Engine->CommandQueue()->ExecuteCommandLists(1, cmdLists);
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

    m_srvDesc.Format = resDesc.Format;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    m_srvDesc.Texture2D.MipLevels = static_cast<UINT>(mipMeta.mipLevels);
    m_srvDesc.Texture2D.MostDetailedMip = 0;
    return true;
}

bool Texture2D::InternalCreateFromData(const uint8_t* data, size_t dataSize, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    m_resource = GetDefaultResource(width, height);
    if (m_resource == nullptr) {
        return false;
    }

    try
    {
        ThrowIfFailed(m_resource->WriteToSubresource(
            0,
            nullptr,
            data,
            static_cast<UINT>(width * 4),
            static_cast<UINT>(dataSize)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    m_srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    m_srvDesc.Texture2D.MipLevels = 1;

    return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Texture2D::GetDefaultResource(size_t width, size_t height)
{
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

    Microsoft::WRL::ComPtr<ID3D12Resource> buff = nullptr;

    auto result = g_Engine->Device()->CreateCommittedResource(
        &texHeapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&buff)
    );

    if (FAILED(result))
    {
        assert(SUCCEEDED(result));
        return nullptr;
    }
    return buff;
}
