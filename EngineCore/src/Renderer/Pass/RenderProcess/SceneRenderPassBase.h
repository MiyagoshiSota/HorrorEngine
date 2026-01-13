#pragma once
#include <vector>

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"

class GameObject;

class SceneRenderPassBase : public IRenderPass
{
public:
    void Execute(RenderContext& context) override
    {
        // このパスで描画するオブジェクトを収集する
        Collect(context);

        // 収集したオブジェクトを描画する
        Draw(context);
    }

    void LastExecute(RenderContext& context)
    {
        // このパスで描画するオブジェクトを収集する
        Collect(context);
        // 収集したオブジェクトを描画する
        Draw(context);
	}

protected:
    // 派生クラスが実装する純粋仮想関数
    virtual void Collect(RenderContext& context) = 0;
    virtual void Draw(RenderContext& context) = 0;
    // 描画対象のオブジェクトを保持するリスト（レンダーキュー）
    std::vector<std::shared_ptr<GameObject>> m_RenderQueue;
};