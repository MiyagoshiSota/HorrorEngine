#pragma once
#include "Renderer/RenderContext/RenderContext.h"

class RenderContext;

class IRenderPass
{
public:
    virtual ~IRenderPass() = default;

    /// <summary>
	/// パスを実行するための純粋仮想関数
    /// </summary>
    /// <param name="context"></param>
    virtual void Execute(RenderContext& context) = 0;
};
