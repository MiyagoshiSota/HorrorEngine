#pragma once
#include <d3d12.h>
#include "Renderer/RenderContext/RenderContext.h"

class IRenderPass
{
public:
    virtual ~IRenderPass() = default;

    // このパスを実行する
    // RenderPipelineから必要な情報(コマンドリストなど)を受け取る
    virtual void Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context) = 0;
};
