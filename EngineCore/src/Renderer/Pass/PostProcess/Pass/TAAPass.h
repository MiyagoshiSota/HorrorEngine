#pragma once

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"
#include "Modules/PublicConst/ConstRenderPref.h"

struct TAAPassParams
{
	float invWidth;
	float invHeight;
	float blendFactor;
	float historyWeight;
};

/// 時間軸型 AA (TAA)。履歴バッファと現在フレームをブレンドして時間的整合性を向上。
class TAAPass : public PostProcessPassBase
{
public:
	TAAPass() : PostProcessPassBase("TAA", "PostProcess_MultiTextureAndCBV")
	{
		SetPassConstantBuffer(std::make_shared<ConstantBuffer>(sizeof(TAAPassParams)));
	}

	void ApplyParameters(
		ID3D12GraphicsCommandList* cmdList,
		RenderContext& context,
		std::shared_ptr<ITargetBase> inputRT,
		const PostProcessParameter& params) override
	{
		auto* shaderParams = GetPassConstantBuffer()->GetPtr<TAAPassParams>();
		shaderParams->invWidth = 1.0f / context.ScreenWidth;
		shaderParams->invHeight = 1.0f / context.ScreenHeight;
		shaderParams->blendFactor = 0.1f;
		shaderParams->historyWeight = 0.4f; // 履歴を90%使用

		cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());
		
		// PostProcess_MultiTexture: t0 = current frame, t1 = history buffer
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
		
		// 履歴バッファを取得
		auto historyRT = context.GetRenderTarget(ConstRenderPref::HistoryBuffer);
		if (historyRT && historyRT->GetSRVHandle())
		{
			cmdList->SetGraphicsRootDescriptorTable(2, historyRT->GetSRVHandle()->gpuHandle);
		}
	}
};
