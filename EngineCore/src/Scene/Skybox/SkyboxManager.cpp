#include "SkyboxManager.h"

#include "Renderer/Pass/RenderProcess/Pass/SkyboxPass.h"
#include "Renderer/Texture/TextureResourceManager.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Renderer/Engine.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHeap.h"
#include <d3dx12.h>

using namespace DirectX;

SkyboxManager::SkyboxManager()
    : m_cubeMap(nullptr)
    , m_srvHandle(nullptr)
{
    CreateCubeMesh();
    CreateConstantBuffer();
    m_initialized = true;
}

bool SkyboxManager::LoadCubeMap(const std::wstring& cubeMapPath)
{
    m_cubeMapPath = cubeMapPath;

    // TextureResourceManager経由でキューブマップをロード（キャッシュ有効）
    m_cubeMap = TextureResourceManager::Instance().GetCubeMap(cubeMapPath);
    if (!m_cubeMap)
    {
        printf("Skyboxキューブマップのロードに失敗: %ls\n", cubeMapPath.c_str());
        return false;
    }

    // SRVハンドルを確保
    m_srvHandle = g_Engine->GetDescriptorHeap()->Allocate(1);
    if (!m_srvHandle)
    {
        printf("Skybox SRVハンドルの確保に失敗\n");
        m_cubeMap = nullptr;
        return false;
    }

    // SRVを作成
    g_Engine->Device()->CreateShaderResourceView(
        m_cubeMap->GetResource(),
        &m_cubeMap->GetViewDesc(),
        m_srvHandle->cpuHandle
    );

    return true;
}

bool SkyboxManager::LoadCubeMap(const std::string& cubeMapPath)
{
    // 文字列をワイド文字列に変換
    std::wstring wpath(cubeMapPath.begin(), cubeMapPath.end());
    return LoadCubeMap(wpath);
}


SkyboxManager::RenderData SkyboxManager::GetRenderData() const
{
    RenderData data = {};
    data.vertexBuffer = m_vertexBuffer.get();
    data.indexBuffer = m_indexBuffer.get();
    data.indexCount = m_indexCount;
    data.constantBuffer = m_constantBuffer.get();
    data.srvHandle = m_srvHandle.get();
    return data;
}

void SkyboxManager::UpdateConstantBuffer(XMMATRIX viewProj)
{
    struct alignas(256) SkyboxCB
    {
        XMMATRIX ViewProj;
    };

    auto pCB = m_constantBuffer->GetPtr<SkyboxCB>();
    pCB->ViewProj = XMMatrixTranspose(viewProj);
}

void SkyboxManager::CreateCubeMesh()
{
    // Skybox用のキューブ頂点（SharedStruct::Vertex形式、内側から見る）
    // シェーダーでz = wに設定し、深度テストLESS_EQUALで最奥に描画
    // Normal, UV, Tangent, Colorはダミー値（シェーダーではPositionのみ使用）
    const XMFLOAT3 dummyNormal = { 0.0f, 0.0f, 0.0f };
    const XMFLOAT2 dummyUV = { 0.0f, 0.0f };
    const XMFLOAT3 dummyTangent = { 0.0f, 0.0f, 0.0f };
    const XMFLOAT4 dummyColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    std::vector<SharedStruct::Vertex> vertices = {
        // 前面 (-Z)
        {{ -1.0f, -1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{ -1.0f,  1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f,  1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f, -1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },

        // 背面 (+Z)
        {{ -1.0f, -1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f, -1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f,  1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{ -1.0f,  1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },

        // 上面 (+Y)
        {{ -1.0f,  1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{ -1.0f,  1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f,  1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f,  1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },

        // 底面 (-Y)
        {{ -1.0f, -1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f, -1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f, -1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{ -1.0f, -1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },

        // 左面 (-X)
        {{ -1.0f, -1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{ -1.0f,  1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{ -1.0f,  1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{ -1.0f, -1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },

        // 右面 (+X)
        {{  1.0f, -1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f,  1.0f, -1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f,  1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
        {{  1.0f, -1.0f,  1.0f }, dummyNormal, dummyUV, dummyTangent, dummyColor },
    };

    // インデックス（内側から見るため、ワインディングを反転）
    std::vector<uint32_t> indices = {
        // 前面 (-Z) - 反転
        0, 2, 1, 0, 3, 2,
        // 背面 (+Z) - 反転
        4, 6, 5, 4, 7, 6,
        // 上面 (+Y) - 反転
        8, 10, 9, 8, 11, 10,
        // 底面 (-Y) - 反転
        12, 14, 13, 12, 15, 14,
        // 左面 (-X) - 反転
        16, 18, 17, 16, 19, 18,
        // 右面 (+X) - 反転
        20, 22, 21, 20, 23, 22
    };

    m_indexCount = static_cast<uint32_t>(indices.size());

    m_vertexBuffer = std::make_unique<VertexBuffer>(
        vertices.size() * sizeof(SharedStruct::Vertex),
        sizeof(SharedStruct::Vertex),
        vertices.data()
    );

    m_indexBuffer = std::make_unique<IndexBuffer>(
        indices.size() * sizeof(uint32_t),
        indices.data()
    );
}

void SkyboxManager::CreateConstantBuffer()
{
    struct alignas(256) SkyboxCB
    {
        XMMATRIX ViewProj;
    };

    m_constantBuffer = std::make_unique<ConstantBuffer>(sizeof(SkyboxCB));
}

bool SkyboxManager::SetupPass(std::shared_ptr<SkyboxPass> skyboxPass)
{
    if (!IsValid())
    {
        printf("SkyboxManager::SetupPass: Skyboxが有効ではありません\n");
        return false;
    }
    if (!skyboxPass)
    {
        printf("SkyboxManager::SetupPass: SkyboxPassがnullです\n");
        return false;
    }
    // 何もしない（RenderContext経由でデータが渡される）
    return true;
}

bool SkyboxManager::LoadAndSetup(const std::wstring& cubeMapPath, std::shared_ptr<SkyboxPass> skyboxPass)
{
    if (!LoadCubeMap(cubeMapPath))
    {
        return false;
    }

    return SetupPass(skyboxPass);
}

bool SkyboxManager::LoadAndSetup(const std::string& cubeMapPath, std::shared_ptr<SkyboxPass> skyboxPass)
{
    // 文字列をワイド文字列に変換
    std::wstring wpath(cubeMapPath.begin(), cubeMapPath.end());
    return LoadAndSetup(wpath, skyboxPass);
}
