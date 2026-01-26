#include "TextureResourceManager.h"
#include <iostream> // ログ用

std::shared_ptr<Texture2D> TextureResourceManager::GetTexture(const std::wstring path)
{
    // キャッシュにあるか確認
    auto it = m_textureCache.find(path);
    if (it != m_textureCache.end())
    {
        return it->second;
    }

    // キャッシュになければ新規作成
    std::shared_ptr<Texture2D> newTex = Texture2D::Load(path);

    if (newTex)
    {
        // 成功したらキャッシュ登録
        m_textureCache[path] = newTex;
        return newTex;
    }

    // ロード失敗時
    // エラーログを出して、nullまたはデフォルトの白テクスチャを返すなどの処理
	printf("テクスチャのロードに失敗: %ls\n", path.c_str());
    return nullptr;
}

std::shared_ptr<Texture2D> TextureResourceManager::WhiteTexture()
{

    // まだ作成されていなければ作成する（遅延初期化）
    if (!m_whiteTexture)
    {
        // Texture2D側に、ファイルからではなくプログラムで白画像を生成する関数を用意して呼ぶ
        m_whiteTexture = Texture2D::CreateWhiteTexture();
    }

    return m_whiteTexture;
}

std::shared_ptr<TextureCube> TextureResourceManager::GetCubeMap(const std::wstring& path)
{
    // キャッシュにあるか確認
    auto it = m_cubeMapCache.find(path);
    if (it != m_cubeMapCache.end())
    {
        return it->second;
    }

    // キャッシュになければ新規作成
    std::shared_ptr<TextureCube> newCubeMap = TextureCube::Load(path);

    if (newCubeMap)
    {
        // 成功したらキャッシュ登録
        m_cubeMapCache[path] = newCubeMap;
        return newCubeMap;
    }

    // ロード失敗時
    printf("キューブマップのロードに失敗: %ls\n", path.c_str());
    return nullptr;
}

void TextureResourceManager::Clear()
{
    m_textureCache.clear();
    m_cubeMapCache.clear();
}