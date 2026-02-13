#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "Texture2D.h"
#include "TextureCube.h"

class TextureResourceManager
{
public:
    // シングルトン取得
    static TextureResourceManager& Instance()
    {
        static TextureResourceManager instance;
        return instance;
    }

    // コピー禁止
    TextureResourceManager(const TextureResourceManager&) = delete;
    TextureResourceManager& operator=(const TextureResourceManager&) = delete;

    // テクスチャ取得のメイン関数
    // まだロードされていなければcreateTextureし、ロード済みならキャッシュを返す
    std::shared_ptr<Texture2D> GetTexture(const std::wstring path);

    /// GLB埋め込みテクスチャなど、パスでロードできないテクスチャをキャッシュに登録する
    void RegisterTexture(const std::wstring& path, std::shared_ptr<Texture2D> tex);
    std::shared_ptr<Texture2D> GetTexture(const std::string path)
    {
        // 文字列をワイド文字列に変換
        std::wstring wpath(path.begin(), path.end());
        return GetTexture(wpath);
	}

    std::shared_ptr<Texture2D> WhiteTexture();

    /// <summary>
    /// キューブマップテクスチャを取得
    /// まだロードされていなければロードし、ロード済みならキャッシュを返す
    /// </summary>
    /// <param name="path">DDSキューブマップファイルのパス</param>
    /// <returns>成功時: TextureCubeのshared_ptr, 失敗時: nullptr</returns>
    std::shared_ptr<TextureCube> GetCubeMap(const std::wstring& path);
    std::shared_ptr<TextureCube> GetCubeMap(const std::string& path)
    {
        // 文字列をワイド文字列に変換
        std::wstring wpath(path.begin(), path.end());
        return GetCubeMap(wpath);
    }

    // リソースの解放
    void Clear();

private:
    TextureResourceManager() = default;
    ~TextureResourceManager() = default;

private:
    // パスをキーにしたキャッシュ
    std::unordered_map<std::wstring, std::shared_ptr<Texture2D>> m_textureCache;
    std::unordered_map<std::wstring, std::shared_ptr<TextureCube>> m_cubeMapCache;
    std::shared_ptr<Texture2D> m_whiteTexture;
};