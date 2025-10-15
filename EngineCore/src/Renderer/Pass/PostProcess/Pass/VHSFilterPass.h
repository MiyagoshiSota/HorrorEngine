#pragma once
#include <d3d12.h>

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"

class VHSFilterPass : public PostProcessPassBase
{
public:
	VHSFilterPass() : PostProcessPassBase("VHSFilter", "PostProcess_TextureAndCBV") {}
	void ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT) override
	{
		// SRVとCBVの設定
		auto timeConstantBuffer = g_Scene->get_time_manager()->get_constant_buffer();
		cmdList->SetGraphicsRootConstantBufferView(0, timeConstantBuffer->GetAddress());
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
	};
};

