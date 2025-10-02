#pragma once
#include "IRenderPass.h"

class SceneSetupPass : public IRenderPass
{
public:
    void Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context) override;
};
