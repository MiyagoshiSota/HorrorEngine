#pragma once

#include <map>
#include <string>
#include <vector>
#include <algorithm> // for std::find_if
#include <memory>
#include <d3d12.h>   // for ID3D12Device, DXGI_FORMAT

#include "TempRenderTargetPool.h"
#include "Renderer/Target/RenderTarget.h" 
#include "Scene/Camera/SceneCamera.h"
#include "Renderer/PipelineManager/PipelineStateManager.h"
#include "Scene/GameObject/GameObject.h"

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

private:
    std::map<std::string, std::shared_ptr<ITargetBase>> m_TargetPool;
    std::shared_ptr<TempRenderTargetPool> m_ExternalTempPool;
};