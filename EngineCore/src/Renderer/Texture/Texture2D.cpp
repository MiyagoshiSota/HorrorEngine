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

    const Image* img = scratch.GetImage(0, 0, 0);

    // GPUリソース作成（DEFAULT ヒープ、COPY_DEST状態）
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        meta.format,
        meta.width,
        meta.height,
        static_cast<UINT16>(meta.arraySize),
        static_cast<UINT16>(meta.mipLevels)
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

    // レイアウト計算
    UINT64 uploadBufferSize = 0;
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};

    g_Engine->Device()->GetCopyableFootprints(
        &resDesc,
        0,
        1,
        0,
        &layout,
        &numRows,
        &rowSizeInBytes,
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

        const uint8_t* pSrcData = img->pixels;
        const UINT srcRowPitch = static_cast<UINT>(img->rowPitch);
        const UINT rowSize = static_cast<UINT>(rowSizeInBytes);

        for (UINT row = 0; row < numRows; ++row)
        {
            memcpy(pData + layout.Offset + row * layout.Footprint.RowPitch,
                   pSrcData + row * srcRowPitch,
                   rowSize);
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
        CD3DX12_TEXTURE_COPY_LOCATION dst(m_resource.Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION src(uploadBuffer.Get(), layout);
        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

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
    m_srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

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
