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
#include "Scene/RayTracing/RayTracedShadowManager.h"
#include "Scene/RayTracing/RayTracedAOManager.h"
#include "Scene/RayTracing/RayTracedGIManager.h"
#include "Renderer/RenderContext/ShadowTypes.h"

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
        , m_taaJitter(0.0f, 0.0f)
        , m_taaEnabled(false)
        , m_useRayTracedShadow(false)
        , m_invRayTracedShadowMapSize(0.0f, 0.0f)
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

    // --- Ray Traced Shadowデータ管理 ---
    void SetRayTracedShadowData(const RayTracedShadowRenderData& data)
    {
        m_rayTracedShadowData = data;
    }

    const RayTracedShadowRenderData& GetRayTracedShadowData() const
    {
        return m_rayTracedShadowData;
    }

    void SetRayTracedShadowUpdateCallback(std::function<void(const RayTracedShadowSceneConstants&, UINT)> callback)
    {
        m_rayTracedShadowUpdateCallback = callback;
    }

    void UpdateRayTracedShadowConstants(const RayTracedShadowSceneConstants& constants, UINT frameIndex)
    {
        if (m_rayTracedShadowUpdateCallback)
        {
            m_rayTracedShadowUpdateCallback(constants, frameIndex);
        }
    }

    void SetUseRayTracedShadow(bool use) { m_useRayTracedShadow = use; }
    bool UseRayTracedShadow() const { return m_useRayTracedShadow; }
    void SetInvRayTracedShadowMapSize(float invW, float invH) { m_invRayTracedShadowMapSize = DirectX::XMFLOAT2(invW, invH); }
    DirectX::XMFLOAT2 GetInvRayTracedShadowMapSize() const { return m_invRayTracedShadowMapSize; }

    // --- Ray Traced AO データ管理 ---
    void SetRayTracedAOData(const RayTracedAORenderData& data) { m_rayTracedAOData = data; }
    const RayTracedAORenderData& GetRayTracedAOData() const { return m_rayTracedAOData; }
    void SetRayTracedAOUpdateCallback(std::function<void(const RayTracedAOConstants&, UINT)> callback) { m_rayTracedAOUpdateCallback = callback; }
    void UpdateRayTracedAOConstants(const RayTracedAOConstants& constants, UINT frameIndex)
    {
        if (m_rayTracedAOUpdateCallback) m_rayTracedAOUpdateCallback(constants, frameIndex);
    }

    // --- Ray Traced GI データ管理 ---
    void SetRayTracedGIData(const RayTracedGIRenderData& data) { m_rayTracedGIData = data; }
    const RayTracedGIRenderData& GetRayTracedGIData() const { return m_rayTracedGIData; }
    void SetRayTracedGIUpdateCallback(std::function<void(const RayTracedGIConstants&, UINT)> callback) { m_rayTracedGIUpdateCallback = callback; }
    void UpdateRayTracedGIConstants(const RayTracedGIConstants& constants, UINT frameIndex)
    {
        if (m_rayTracedGIUpdateCallback) m_rayTracedGIUpdateCallback(constants, frameIndex);
    }

    void SetRTGIEnabled(bool enabled) { m_rtgiEnabled = enabled; }
    bool IsRTGIEnabled() const { return m_rtgiEnabled; }

    /// シャドウ契約（ライティングパスが参照）
    void SetShadowContext(const ShadowContext& sc)
    {
        m_shadowContext = sc;
        m_useRayTracedShadow = (sc.mode == ShadowMode::RayTracedMask || sc.mode == ShadowMode::RayTracedVisibility);
    }
    const ShadowContext& GetShadowContext() const { return m_shadowContext; }

    /// <summary>
    /// TAAジッターを設定
    /// </summary>
    void SetTAAJitter(DirectX::XMFLOAT2 jitter, bool enabled)
    {
        m_taaJitter = jitter;
        m_taaEnabled = enabled;
    }

    /// <summary>
    /// TAAジッターを適用した投影行列を取得（TAAが有効な場合のみ適用）
    /// </summary>
    DirectX::XMMATRIX GetProjectionMatrix() const
    {
        DirectX::XMMATRIX proj = Camera->GetProjectionMatrix();
        
        // TAA無効時は通常の投影行列を返す
        if (!m_taaEnabled)
        {
            return proj;
        }
        
        // ジッターをNDC空間（[-1, 1]）に変換
        // jitterはピクセル単位の[-0.5, 0.5]なので、スクリーン解像度で正規化して2倍
        float jitterX = (m_taaJitter.x / ScreenWidth) * 2.0f;
        float jitterY = (m_taaJitter.y / ScreenHeight) * 2.0f;
        
        // 投影行列の平行移動成分にジッターを適用
        // proj[2][0] = jitterX (X軸オフセット)
        // proj[2][1] = jitterY (Y軸オフセット)
        DirectX::XMFLOAT4X4 projMatrix;
        DirectX::XMStoreFloat4x4(&projMatrix, proj);
        projMatrix._31 += jitterX;
        projMatrix._32 += jitterY;
        
        return DirectX::XMLoadFloat4x4(&projMatrix);
    }

    /// <summary>
    /// ジッターなしの純粋な投影行列（モーションベクター/カリング用）
    /// </summary>
    DirectX::XMMATRIX GetNonJitteredProjectionMatrix() const
    {
        // カメラの元の投影行列をそのまま返す
        return Camera->GetProjectionMatrix();
    }

private:
    std::map<std::string, std::shared_ptr<ITargetBase>> m_TargetPool;
    std::shared_ptr<TempRenderTargetPool> m_ExternalTempPool;
    SkyboxData m_skyboxData;
    std::function<void(DirectX::XMMATRIX)> m_skyboxUpdateCallback;
    RayTracedShadowRenderData m_rayTracedShadowData;
    std::function<void(const RayTracedShadowSceneConstants&, UINT)> m_rayTracedShadowUpdateCallback;
    RayTracedAORenderData m_rayTracedAOData;
    std::function<void(const RayTracedAOConstants&, UINT)> m_rayTracedAOUpdateCallback;
    RayTracedGIRenderData m_rayTracedGIData;
    std::function<void(const RayTracedGIConstants&, UINT)> m_rayTracedGIUpdateCallback;
    bool m_rtgiEnabled = false;
    DirectX::XMFLOAT2 m_taaJitter;
    bool m_taaEnabled;
    bool m_useRayTracedShadow;
    DirectX::XMFLOAT2 m_invRayTracedShadowMapSize;
    ShadowContext m_shadowContext;
};