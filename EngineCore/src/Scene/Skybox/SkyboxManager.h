#pragma once
#include <memory>
#include <string>
#include "Renderer/Texture/TextureCube.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"
#include "Renderer/Graphics/Buffer/VertexBuffer.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include <DirectXMath.h>

class SkyboxPass;

/// <summary>
/// Skyboxのリソース管理と設定を行うマネージャー
/// キューブマップのロード、SRV作成、メッシュデータ、定数バッファの管理を担当
/// </summary>
class SkyboxManager
{
public:
    SkyboxManager();
    ~SkyboxManager() = default;

    /// <summary>
    /// Skybox用のキューブマップをロードして設定
    /// </summary>
    /// <param name="cubeMapPath">DDSキューブマップファイルのパス</param>
    /// <returns>成功した場合true</returns>
    bool LoadCubeMap(const std::wstring& cubeMapPath);
    bool LoadCubeMap(const std::string& cubeMapPath);

    /// <summary>
    /// SkyboxPassにSkyboxManagerへの参照を設定
    /// </summary>
    /// <param name="skyboxPass">設定対象のSkyboxPass</param>
    /// <returns>成功した場合true</returns>
    bool SetupPass(std::shared_ptr<SkyboxPass> skyboxPass);

    /// <summary>
    /// キューブマップをロードしてSkyboxPassに設定する（便利メソッド）
    /// LoadCubeMap()とSetupPass()を順に実行する
    /// </summary>
    /// <param name="cubeMapPath">DDSキューブマップファイルのパス</param>
    /// <param name="skyboxPass">設定対象のSkyboxPass</param>
    /// <returns>成功した場合true</returns>
    bool LoadAndSetup(const std::wstring& cubeMapPath, std::shared_ptr<SkyboxPass> skyboxPass);
    bool LoadAndSetup(const std::string& cubeMapPath, std::shared_ptr<SkyboxPass> skyboxPass);

    /// <summary>
    /// Skyboxが有効かどうか
    /// </summary>
    bool IsValid() const { return m_cubeMap != nullptr && m_srvHandle != nullptr && m_initialized; }

    /// <summary>
    /// 現在設定されているキューブマップのパスを取得
    /// </summary>
    const std::wstring& GetCubeMapPath() const { return m_cubeMapPath; }

    /// <summary>
    /// 描画に必要なデータを取得（SkyboxPass用）
    /// </summary>
    struct RenderData
    {
        VertexBuffer* vertexBuffer;
        IndexBuffer* indexBuffer;
        uint32_t indexCount;
        ConstantBuffer* constantBuffer;
        DescriptorHandle* srvHandle;
    };
    RenderData GetRenderData() const;

    /// <summary>
    /// 定数バッファを更新（SkyboxPass用）
    /// </summary>
    void UpdateConstantBuffer(DirectX::XMMATRIX viewProj);

private:
    void CreateCubeMesh();
    void CreateConstantBuffer();

private:
    bool m_initialized = false;
    std::wstring m_cubeMapPath;

    // キューブマップテクスチャ
    std::shared_ptr<TextureCube> m_cubeMap;
    std::shared_ptr<DescriptorHandle> m_srvHandle;

    // キューブメッシュ
    std::unique_ptr<VertexBuffer> m_vertexBuffer;
    std::unique_ptr<IndexBuffer> m_indexBuffer;
    uint32_t m_indexCount = 0;

    // 定数バッファ
    std::unique_ptr<ConstantBuffer> m_constantBuffer;
};
