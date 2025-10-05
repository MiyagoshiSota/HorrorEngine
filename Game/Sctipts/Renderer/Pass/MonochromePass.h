#pragma once
#include "Renderer/RenderPass/IRenderPass.h"

class MonochromePass : public IRenderPass
{
public:
	void Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context) override;
};

