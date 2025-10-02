#pragma once
#include "IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"

class GeometryPass : public IRenderPass
{
public:
    void Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context) override;
};
