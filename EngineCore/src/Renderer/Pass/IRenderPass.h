#pragma once
#include "Renderer/RenderContext/RenderContext.h"

class RenderContext;

class IRenderPass
{
public:
    virtual ~IRenderPass() = default;

    // このパスを実行する
    // RenderPipelineから必要な情報(コマンドリストなど)を受け取る
    virtual void Execute(RenderContext& context) = 0;
};
