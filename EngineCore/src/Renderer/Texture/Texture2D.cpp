#include "Texture2D.h"
#include <DirectXTex.h>
#include "Renderer/Engine.h"

#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

std::unordered_map<std::wstring, std::shared_ptr<Texture2D>> Texture2D::m_TextureCache;
std::shared_ptr<Texture2D> Texture2D::m_WhiteTexture;

// std::string(マルチバイト文字列)からstd::wstring(ワイド文字列)を得る。AssimpLoaderと同じものだけど、共用にするのがめんどくさかったので許してください
std::wstring GetWideString(const std::string& str)
{
    auto num1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);

    std::wstring wstr;
    wstr.resize(num1);

    auto num2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, &wstr[0], num1);

    assert(num1 == num2);
    return wstr;
}

// 拡張子を返す
std::wstring FileExtension(const std::wstring& path)
{
    auto idx = path.rfind(L'.');
    return path.substr(idx + 1, path.length() - idx - 1);
}

Texture2D::Texture2D(std::string path)
{
    m_IsValid = Load(path);
}

Texture2D::Texture2D(std::wstring path)
{
    m_IsValid = Load(path);
}

Texture2D::Texture2D(ComPtr < ID3D12Resource > buffer)
{
    m_pResource = buffer;
    m_IsValid = m_pResource != nullptr;
}

// 戻り値をshared_ptrに変更
std::shared_ptr<Texture2D> Texture2D::Get(std::string path)
{
    auto wpath = GetWideString(path);
    return Get(wpath);
}

// Getメソッドをキャッシュ対応に修正
std::shared_ptr<Texture2D> Texture2D::Get(std::wstring path)
{
    // 1. キャッシュにテクスチャがあるか探す
    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end())
    {
        // あればそれを返す
        return it->second;
    }

    // 2. なければ新しく作る (make_sharedを使う)
    auto tex = std::make_shared<Texture2D>(path);
    if (!tex->IsValid())
    {
        return GetWhite(); // 失敗したら白テクスチャを返す
    }

    // 3. 作成したテクスチャをキャッシュに保存
    m_TextureCache[path] = tex;
    return tex;
}

// GetWhiteもキャッシュ対応に
std::shared_ptr<Texture2D> Texture2D::GetWhite()
{
    if (m_WhiteTexture)
    {
        return m_WhiteTexture;
    }

    const size_t width = 4;
    const size_t height = 4;

    // 1. リソースを確保
    ComPtr<ID3D12Resource> buff = GetDefaultResource(width, height);
    if (buff == nullptr) {
        return nullptr;
    }

    // 2. 4x4ピクセル分の白色データを作成 (RGBA, 各色255)
    std::vector<UINT8> data(width * height * 4, 255);

    // 3. 作成したデータをリソースに書き込む
    HRESULT hr = buff->WriteToSubresource(
        0,
        nullptr,
        data.data(),
        static_cast<UINT>(width * 4), // 1ラインのバイト数 (幅 * 4チャンネル)
        static_cast<UINT>(data.size()) // 全体のバイト数
    );
    
    if (FAILED(hr)) {
        // エラー処理
        return nullptr;
    }

    // 4. データが書き込まれたリソースでテクスチャオブジェクトを作成
    m_WhiteTexture = std::make_shared<Texture2D>(buff);
    return m_WhiteTexture;
}

bool Texture2D::IsValid()
{
    return m_IsValid;
}

ComPtr<ID3D12Resource> Texture2D::Resource()
{
    return m_pResource.Get();
}

D3D12_SHADER_RESOURCE_VIEW_DESC Texture2D::ViewDesc()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = m_pResource->GetDesc().Format;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; //2Dテクスチャ
    desc.Texture2D.MipLevels = 1; //ミップマップは使用しないので1
    return desc;
}

bool Texture2D::Load(std::string& path)
{
    auto wpath = GetWideString(path);
    return Load(wpath);
}

bool Texture2D::Load(std::wstring& path)
{
    //WICテクスチャのロード
    TexMetadata meta = {};
    ScratchImage scratch = {};
    auto ext = FileExtension(path);

    HRESULT hr = S_FALSE;
    if (ext == L"png") // pngの時はWICFileを使う
    {
        LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &meta, scratch);
    }
    else if (ext == L"tga") // tgaの時はTGAFileを使う
    {
        hr = LoadFromTGAFile(path.c_str(), &meta, scratch);
    }

    if (FAILED(hr))
    {
        return false;
    }

    auto img = scratch.GetImage(0, 0, 0);
    auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(meta.format,
        meta.width,
        meta.height,
        static_cast<UINT16>(meta.arraySize),
        static_cast<UINT16>(meta.mipLevels));

    // リソースを生成
    hr = g_Engine->Device()->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(m_pResource.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        return false;
    }

    hr = m_pResource->WriteToSubresource(0,
        nullptr, //全領域へコピー
        img->pixels, //元データアドレス
        static_cast<UINT>(img->rowPitch), //1ラインサイズ
        static_cast<UINT>(img->slicePitch) //全サイズ
    );
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

ComPtr<ID3D12Resource> Texture2D::GetDefaultResource(size_t width, size_t height)
{
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
    ComPtr<ID3D12Resource> buff = nullptr;
    auto result = g_Engine->Device()->CreateCommittedResource(
        &texHeapProp,
        D3D12_HEAP_FLAG_NONE, //特に指定なし
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

