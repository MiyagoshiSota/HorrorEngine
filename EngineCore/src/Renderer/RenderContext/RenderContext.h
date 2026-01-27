#pragma once

#include <map>
#include <string>
#include <vector>
#include <algorithm> // for std::find_if
#include <memory>
#include <functional>
#include <d3d12.h>   // for ID3D12Device, DXGI_FORMAT
#include <DirectXMath.h>

#include "TempRenderTargetPool.h"
#include "Renderer/Target/RenderTarget.h" 
#include "Scene/Camera/SceneCamera.h"
#include "Renderer/PipelineManager/PipelineStateManager.h"
#include "Scene/GameObject/GameObject.h"
#include "Renderer/Graphics/Buffer/VertexBuffer.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include "Renderer/Graphics/DescriptorHeap/DescriptorHandle.h"

class IPipelineManager;
struct ID3D12GraphicsCommandList;

class RenderContext
{
public:
    // Deviceを追加 (リソース生成のため)
    RenderContext(
        ID3D12GraphicsCommandList* cmdList,
        std::shared_ptr<SceneCamera> camera,
        std::vector<std::shared_ptr <GameObject>> gameObjects,
        std::shared_ptr<ITargetBase> sourceRT,
        std::shared_ptr<ITargetBase> destRT,
        float width,
        float height,
        std::shared_ptr<PipelineStateManager> pipelineStateManager,
        std::shared_ptr<TempRenderTargetPool> persistentPool // <--- 追加！
    )
        : CommandList(cmdList)
        , Camera(camera)
        , GameObjects(gameObjects)
        , SourceRT(sourceRT)
        , DestRT(destRT)
        , ScreenWidth(width)
        , ScreenHeight(height)
        , PipelineStateManager(pipelineStateManager)
        , m_ExternalTempPool(persistentPool) // <--- 参照を保持
    {
        Device = g_Engine->Device();
    }

    // --- 各パスが必要とする情報 ---
    ID3D12Device* Device;
    ID3D12GraphicsCommandList* CommandList;
    std::shared_ptr<SceneCamera> Camera;
    std::vector<std::shared_ptr <GameObject>> GameObjects;
    std::shared_ptr<PipelineStateManager> PipelineStateManager;
    std::shared_ptr<ITargetBase> SourceRT;
    std::shared_ptr<ITargetBase> DestRT;
    std::shared_ptr<ITargetBase> MSAART;
    float ScreenWidth;
    float ScreenHeight;

public:
    // --- 名前付きレンダーターゲット管理--
    std::shared_ptr<ITargetBase> GetRenderTarget(const std::string& name)
    {
        if (m_TargetPool.count(name)) {
            return m_TargetPool[name];
        }
        return nullptr;
    }

    void AddRenderTarget(const std::string& name, std::shared_ptr<ITargetBase> target)
    {
        m_TargetPool[name] = target;
    }

    void SetSourceRT(std::shared_ptr<ITargetBase> rt) { SourceRT = rt; }
    void SetDestRT(std::shared_ptr<ITargetBase> rt) { DestRT = rt; }

    std::shared_ptr<ITargetBase> GetSourceRT() { return SourceRT; }
    std::shared_ptr<ITargetBase> GetDestRT() { return DestRT; }

    // --- 一時レンダーターゲット管理 ---
    std::shared_ptr<RenderTarget> GetTempRenderTarget(float width, float height, DXGI_FORMAT format)
    {
        if (m_ExternalTempPool) return m_ExternalTempPool->Get(width, height, format);
        return nullptr;
    }

    void ReleaseTempRenderTarget(std::shared_ptr<RenderTarget> target)
    {
        if (m_ExternalTempPool) m_ExternalTempPool->Return(target);
    }

    // --- RAII Helper (自動返却用) ---
    // これを使うと ReleaseTempRenderTarget を呼び忘れる心配がありません
    struct ScopedTempTarget
    {
        std::shared_ptr<RenderTarget> Target;
        RenderContext* Context;

        ScopedTempTarget(RenderContext* ctx, float w, float h, DXGI_FORMAT fmt)
            : Context(ctx)
        {
            Target = ctx->GetTempRenderTarget(w, h, fmt);
        }

        ~ScopedTempTarget()
        {
            if (Context && Target)
            {
                Context->ReleaseTempRenderTarget(Target);
            }
        }

        // 矢印演算子で RenderTarget に直接アクセスできるようにする
        RenderTarget* operator->() { return Target.get(); }
        std::shared_ptr<RenderTarget> Get() { return Target; }
    };

    // RAIIオブジェクトを作成するヘルパー関数
    ScopedTempTarget GetScopedTempRT(float width, float height, DXGI_FORMAT format)
    {
        return ScopedTempTarget(this, width, height, format);
    }

    // --- Skyboxデータ管理 ---
    /// <summary>
    /// Skybox描画に必要なデータ構造体
    /// </summary>
    struct SkyboxData
    {
        VertexBuffer* vertexBuffer = nullptr;
        IndexBuffer* indexBuffer = nullptr;
        uint32_t indexCount = 0;
        ConstantBuffer* constantBuffer = nullptr;
        DescriptorHandle* srvHandle = nullptr;
        bool isValid = false;
    };

    /// <summary>
    /// Skyboxデータを設定
    /// </summary>
    void SetSkyboxData(const SkyboxData& data)
    {
        m_skyboxData = data;
    }

    /// <summary>
    /// Skyboxデータを取得
    /// </summary>
    const SkyboxData& GetSkyboxData() const
    {
        return m_skyboxData;
    }

    /// <summary>
    /// Skybox定数バッファ更新用のコールバックを設定
    /// </summary>
    void SetSkyboxUpdateCallback(std::function<void(DirectX::XMMATRIX)> callback)
    {
        m_skyboxUpdateCallback = callback;
    }

    /// <summary>
    /// Skybox定数バッファを更新
    /// </summary>
    void UpdateSkyboxConstantBuffer(DirectX::XMMATRIX viewProj)
    {
        if (m_skyboxUpdateCallback)
        {
            m_skyboxUpdateCallback(viewProj);
        }
    }

private:
    std::map<std::string, std::shared_ptr<ITargetBase>> m_TargetPool;
    std::shared_ptr<TempRenderTargetPool> m_ExternalTempPool;
    SkyboxData m_skyboxData;
    std::function<void(DirectX::XMMATRIX)> m_skyboxUpdateCallback;
};