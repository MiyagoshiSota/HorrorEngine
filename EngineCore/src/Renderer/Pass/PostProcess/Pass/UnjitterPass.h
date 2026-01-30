#pragma once

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"
#include <DirectXMath.h>

struct UnjitterPassParams
{
	float invWidth;
	float invHeight;
	float jitterX; // 現在フレームのジッター（ピクセル単位）
	float jitterY;
};

/// TAA結果からジッターを除去するパス。履歴バッファに保存する前に実行。
class UnjitterPass : public PostProcessPassBase
{
public:
	UnjitterPass() : PostProcessPassBase("Unjitter", "PostProcess_TextureAndCBV")
	{
		SetPassConstantBuffer(std::make_shared<ConstantBuffer>(sizeof(UnjitterPassParams)));
	}

	void ApplyParameters(
		ID3D12GraphicsCommandList* cmdList,
		RenderContext& context,
		std::shared_ptr<ITargetBase> inputRT,
		const PostProcessParameter& params) override
	{
		auto* shaderParams = GetPassConstantBuffer()->GetPtr<UnjitterPassParams>();
		shaderParams->invWidth = 1.0f / context.ScreenWidth;
		shaderParams->invHeight = 1.0f / context.ScreenHeight;
		
		// ジッター値を取得（外部から設定される）
		shaderParams->jitterX = m_jitter.x;
		shaderParams->jitterY = m_jitter.y;

		cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
	}

	/// 現在のジッター値を設定
	void SetJitter(DirectX::XMFLOAT2 jitter)
	{
		m_jitter = jitter;
	}

private:
	DirectX::XMFLOAT2 m_jitter;
};
