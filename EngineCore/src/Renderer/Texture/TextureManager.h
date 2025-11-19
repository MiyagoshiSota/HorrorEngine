#pragma once
#include <string>
#include <memory>
#include <unordered_map>

class Texture2D;

/// <summary>
/// Texture2Dを管理するマネージャークラス
/// テクスチャのロード、キャッシュ、取得を管理する
/// </summary>
class TextureManager
{
public:
    TextureManager();
    ~TextureManager();

    /// <summary>
    /// パス(std::string)からテクスチャを取得する
    /// キャッシュにあればそれを返し、なければ新規作成する
    /// </summary>
    std::shared_ptr<Texture2D> Get(std::string path);

    /// <summary>
    /// パス(std::wstring)からテクスチャを取得する
    /// キャッシュにあればそれを返し、なければ新規作成する
    /// </summary>
    std::shared_ptr<Texture2D> Get(std::wstring path);

    /// <summary>
    /// 白色の単色テクスチャを取得する
    /// </summary>
    std::shared_ptr<Texture2D> GetWhite();

    /// <summary>
    /// キャッシュをクリアする
    /// </summary>
    void ClearCache();

private:
    std::unordered_map<std::wstring, std::shared_ptr<Texture2D>> m_TextureCache;
    std::shared_ptr<Texture2D> m_WhiteTexture;

    TextureManager(const TextureManager&) = delete;
    void operator = (const TextureManager&) = delete;
};
