#include "Texture2D.h"
#include <DirectXTex.h>
#include <vector>
#include <cassert>
#include <d3dx12.h>

#include "Renderer/Engine.h"
#include "Modules/DxHelper.h"

// ライブラリのリンク
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

// --- ヘルパー関数 ---

// std::string -> std::wstring 変換
std::wstring GetWideString(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// 拡張子の取得
std::wstring FileExtension(const std::wstring& path)
{
    auto idx = path.rfind(L'.');
    if (idx == std::wstring::npos) return L"";
    return path.substr(idx + 1);
}

// --- コンストラクタ・デストラクタ ---

Texture2D::Texture2D()
    : m_width(0), m_height(0)
{
}

Texture2D::~Texture2D()
{
}

// --- ファクトリーメソッド (外部から呼ばれる) ---

std::shared_ptr<Texture2D> Texture2D::Load(const std::wstring& path)
{
    // インスタンス生成
    auto tex = std::make_shared<Texture2D>();

    // 内部ロード処理を実行
    if (!tex->InternalLoad(path))
    {
        // 失敗したらnullptrを返す
        return nullptr;
    }
    return tex;
}

std::shared_ptr<Texture2D> Texture2D::CreateWhiteTexture()
{
    auto tex = std::make_shared<Texture2D>();

    // 1x1ピクセルの白データ (RGBA = 0xFF, 0xFF, 0xFF, 0xFF)
    uint32_t whitePixel = 0x00000000;
	uint32_t blackPixel = 0x000000FF;

    // 内部メソッドでDX12リソースを作成
    bool success = tex->InternalCreateFromData(
        reinterpret_cast<const uint8_t*>(&whitePixel),
        sizeof(uint32_t),
        1,
        1
    );

    if (!success)
    {
        printf("白テクスチャの作成に失敗\n");
        return nullptr;
    }

    return tex;
}

// --- 内部実装 ---

bool Texture2D::InternalLoad(const std::wstring& path)
{
    m_path = path;

    // DirectXTexを使って画像をロード
    TexMetadata meta = {};
    ScratchImage scratch = {};
    std::wstring ext = FileExtension(path);
    HRESULT hr = E_FAIL;

    // 拡張子による分岐 (小文字化比較などが望ましいですが簡易実装)
    if (ext == L"tga" || ext == L"TGA")
    {
        hr = LoadFromTGAFile(path.c_str(), &meta, scratch);
    }
    else
    {
        // png, jpg, bmp などはWIC
        hr = LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &meta, scratch);
    }

    try
    {
        ThrowIfFailed(hr);
    }
    catch (const std::exception& e)
    {
        printf("画像ロード失敗: %ls - %s\n", path.c_str(), e.what());
        return false;
    }

    // メタデータの保存
    m_width = static_cast<uint32_t>(meta.width);
    m_height = static_cast<uint32_t>(meta.height);

    // リソースの作成 (DirectXTexの情報を元に確保)
    // メモ: GetDefaultResourceはR8G8B8A8_UNORM固定なので、元のフォーマットを使いたい場合は修正が必要
    // ここではシンプルにするため、元コードのWriteToSubresourceパターンに合わせてリソースを作ります

    const Image* img = scratch.GetImage(0, 0, 0);

    // リソースを確保 (フォーマットは画像に合わせる)
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        meta.format,
        meta.width,
        meta.height,
        static_cast<UINT16>(meta.arraySize),
        static_cast<UINT16>(meta.mipLevels)
    );

    auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
            &texHeapProp,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 最初からシェーダーリソースとして使用
            nullptr,
            IID_PPV_ARGS(&m_resource)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    // データの転送 (WriteToSubresource)
    try
    {
        ThrowIfFailed(m_resource->WriteToSubresource(
            0,
            nullptr, // 全領域
            img->pixels,
            static_cast<UINT>(img->rowPitch),
            static_cast<UINT>(img->slicePitch)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    // SRV設定の作成
    m_srvDesc.Format = resDesc.Format;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    m_srvDesc.Texture2D.MipLevels = resDesc.MipLevels;
    // m_srvDesc.Texture2D.MostDetailedMip = 0;
    // m_srvDesc.Texture2D.PlaneSlice = 0;
    // m_srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    return true;
}

bool Texture2D::InternalCreateFromData(const uint8_t* data, size_t dataSize, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    // リソースを確保 (GetDefaultResourceを使用)
    m_resource = GetDefaultResource(width, height);
    if (m_resource == nullptr) {
        return false;
    }

    // データの書き込み
    try
    {
        ThrowIfFailed(m_resource->WriteToSubresource(
            0,
            nullptr,
            data,
            static_cast<UINT>(width * 4), // RowPitch: 1ラインのバイト数 (R8G8B8A8想定)
            static_cast<UINT>(dataSize)   // SlicePitch: 全体のバイト数
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    // SRV設定の作成
    m_srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    m_srvDesc.Texture2D.MipLevels = 1;

    return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Texture2D::GetDefaultResource(size_t width, size_t height)
{
    // R8G8B8A8_UNORM 固定でリソースを作成するヘルパー
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