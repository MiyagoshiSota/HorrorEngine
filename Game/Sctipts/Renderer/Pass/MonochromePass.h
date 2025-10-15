#pragma once
#include "Renderer/Pass/IRenderPass.h"

class MonochromePass : public IRenderPass
{
public:
	void Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context);
};

