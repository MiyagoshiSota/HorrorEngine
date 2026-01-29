#pragma once

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"

struct FXAAPassParams
{
	float invWidth;
	float invHeight;
	float edgeThreshold;
	float edgeThresholdMin;
};

/// FXAA
class FXAAPass : public PostProcessPassBase
{
public:
	FXAAPass() : PostProcessPassBase("FXAA", "PostProcess_TextureAndCBV")
	{
		SetPassConstantBuffer(std::make_shared<ConstantBuffer>(sizeof(FXAAPassParams)));
	}

	void ApplyParameters(
		ID3D12GraphicsCommandList* cmdList,
		RenderContext& context,
		std::shared_ptr<ITargetBase> inputRT,
		const PostProcessParameter& params) override
	{
		auto* shaderParams = GetPassConstantBuffer()->GetPtr<FXAAPassParams>();
		shaderParams->invWidth = 1.0f / context.ScreenWidth;
		shaderParams->invHeight = 1.0f / context.ScreenHeight;
		shaderParams->edgeThreshold = 0.166f;
		shaderParams->edgeThresholdMin = 0.0833f;

		cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
	}
};
