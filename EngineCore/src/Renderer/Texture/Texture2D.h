#pragma once
#include <string>
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>


class Texture2D
{
public:
    Texture2D();
    ~Texture2D();

    // ファクトリーメソッド: パスからロードしてインスタンスを返す
    // (直接newせず、失敗時にnullptrを返せるようにする)
    static std::shared_ptr<Texture2D> Load(const std::wstring& path);
        static std::shared_ptr<Texture2D> CreateWhiteTexture();

    // ゲッター
    const std::wstring& GetPath() const { return m_path; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    // GPUリソースへのアクセサ
    ID3D12Resource* GetResource() const { return m_resource.Get(); }
    const D3D12_SHADER_RESOURCE_VIEW_DESC& GetViewDesc() const { return m_srvDesc; }

private:
    // 実際のロード処理 (内部用)
    bool InternalLoad(const std::wstring& path);
	bool InternalCreateFromData(const uint8_t* data, size_t dataSize, uint32_t width, uint32_t height);
    Microsoft::WRL::ComPtr<ID3D12Resource> GetDefaultResource(size_t width, size_t height);

private:
    std::wstring m_path;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // DirectX 12 リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    // シェーダーリソースビュー(SRV)の設定情報
    D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc = {};
};