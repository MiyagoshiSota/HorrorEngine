#pragma once

#include <map>
#include <string>
#include "Renderer/RenderTarget/RenderTarget.h" 
#include "Scene/Camera/SceneCamera.h"
#include "memory"

struct ID3D12GraphicsCommandList;

class RenderContext
{
public:
    // フレーム開始時に、パイプラインマネージャーが生成する
    RenderContext(ID3D12GraphicsCommandList* cmdList, SceneCamera* camera, float width, float height)
        : CommandList(cmdList)
        , Camera(camera)
        , ScreenWidth(width)
        , ScreenHeight(height)
    {
    }

    // --- 各パスが必要とする情報 ---
    ID3D12GraphicsCommandList* CommandList;
    SceneCamera* Camera;
    float ScreenWidth;
    float ScreenHeight;

    // --- レンダーターゲットの管理機能 ---

    // 名前を指定してレンダーターゲットを取得する
    RenderTarget* GetRenderTarget(const std::string& name)
    {
        if (m_RenderTargetPool.count(name)) {
            return m_RenderTargetPool[name].get();
        }
        return nullptr;
    }

    // パイプラインマネージャーがレンダーターゲットを登録するのに使う
    void AddRenderTarget(const std::string& name, std::unique_ptr<RenderTarget> target)
    {
        m_RenderTargetPool[name] = std::move(target);
    }

private:
    // このフレームで利用可能なレンダーターゲットの一覧
    // 名前でアクセスできるようにmapで管理するのが非常に便利
    std::map<std::string, std::unique_ptr<RenderTarget>> m_RenderTargetPool;
};