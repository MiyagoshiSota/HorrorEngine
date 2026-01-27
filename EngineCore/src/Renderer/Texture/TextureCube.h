#pragma once
#include <string>
#include <memory>
#include <d3d12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

/// <summary>
/// キューブマップテクスチャクラス
/// DDSファイルからキューブマップをロードし、SkyboxなどでSRVとして使用する
/// </summary>
class TextureCube
{
public:
    TextureCube();
    ~TextureCube();

    /// <summary>
    /// DDSファイルからキューブマップをロード
    /// </summary>
    /// <param name="path">DDSファイルへのパス（キューブマップ形式）</param>
    /// <returns>成功時: TextureCubeのshared_ptr, 失敗時: nullptr</returns>
    static std::shared_ptr<TextureCube> Load(const std::wstring& path);

    // ゲッター
    const std::wstring& GetPath() const { return m_path; }
    uint32_t GetSize() const { return m_size; }

    // GPUリソースへのアクセサ
    ID3D12Resource* GetResource() const { return m_resource.Get(); }
    const D3D12_SHADER_RESOURCE_VIEW_DESC& GetViewDesc() const { return m_srvDesc; }

private:
    bool InternalLoad(const std::wstring& path);

private:
    std::wstring m_path;
    uint32_t m_size = 0; // キューブマップの1面のサイズ

    // DirectX 12 リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

    // シェーダーリソースビュー(SRV)の設定情報
    D3D12_SHADER_RESOURCE_VIEW_DESC m_srvDesc = {};
};
