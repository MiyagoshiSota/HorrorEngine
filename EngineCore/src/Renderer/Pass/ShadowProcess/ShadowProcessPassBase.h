#pragma once
#include "Renderer/Pass/IRenderPass.h"

class ShadowProcessPassBase : public IRenderPass
{
public:
    void Execute(RenderContext& context) override
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
};
