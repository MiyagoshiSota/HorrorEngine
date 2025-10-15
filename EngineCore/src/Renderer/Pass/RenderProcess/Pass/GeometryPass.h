#pragma once
#include "Renderer/Pass/RenderProcess/SceneRenderPassBase.h"
#include "Renderer/RenderContext/RenderContext.h"

class GeometryPass : public SceneRenderPassBase
{
protected:
    void Collect(RenderContext& context) override;
    void Draw(RenderContext& context) override;
};