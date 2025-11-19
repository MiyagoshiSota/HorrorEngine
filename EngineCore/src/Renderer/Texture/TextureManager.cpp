#include "TextureManager.h"
#include "Texture2D.h"

// std::string(マルチバイト文字列)からstd::wstring(ワイド文字列)を得る
std::wstring GetWideStringForManager(const std::string& str)
{
    auto num1 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);

    std::wstring wstr;
    wstr.resize(num1);

    auto num2 = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, str.c_str(), -1, &wstr[0], num1);

    assert(num1 == num2);
    return wstr;
}

TextureManager::TextureManager()
    : m_WhiteTexture(nullptr)
{
}

TextureManager::~TextureManager()
{
    ClearCache();
}

std::shared_ptr<Texture2D> TextureManager::Get(std::string path)
{
    auto wpath = GetWideStringForManager(path);
    return Get(wpath);
}

std::shared_ptr<Texture2D> TextureManager::Get(std::wstring path)
{
    // 1. キャッシュにテクスチャがあるか探す
    auto it = m_TextureCache.find(path);
    if (it != m_TextureCache.end())
    {
        // あればそれを返す
        return it->second;
    }

    // 2. なければ新しく作る
    auto tex = std::make_shared<Texture2D>(path);
    if (!tex->IsValid())
    {
        return GetWhite(); // 失敗したら白テクスチャを返す
    }

    // 3. 作成したテクスチャをキャッシュに保存
    m_TextureCache[path] = tex;
    return tex;
}

std::shared_ptr<Texture2D> TextureManager::GetWhite()
{
    if (m_WhiteTexture)
    {
        return m_WhiteTexture;
    }

    // Texture2Dの静的メソッドを使って白テクスチャを生成
    m_WhiteTexture = Texture2D::CreateWhiteTexture();
    return m_WhiteTexture;
}

void TextureManager::ClearCache()
{
    m_TextureCache.clear();
    m_WhiteTexture.reset();
}
