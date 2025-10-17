#pragma once
#include <d3d12.h>

#include "Core/App.h"
#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"

struct VHSPassParams {
	float scanline_intensity;
	float noise_intensity;
};

class VHSPass : public PostProcessPassBase
{
public:
	VHSPass() : PostProcessPassBase("VHS", "PostProcess_TextureAndCBV") {}
	void ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT, const PostProcessParameter& params) override
	{
		// SRVとCBVの設定
		auto timeConstantBuffer = g_Scene->get_time_manager()->get_constant_buffer();
		cmdList->SetGraphicsRootConstantBufferView(0, timeConstantBuffer->GetAddress());
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
	};
};

