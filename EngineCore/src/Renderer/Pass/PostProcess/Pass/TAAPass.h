#pragma once

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include <DirectXMath.h>
#include <d3dx12.h>

struct TAAPassParams
{
	float invWidth;
	float invHeight;
	float blendFactor;
	float historyWeight;
	float currentJitterX; // 現在フレームのジッター（ピクセル単位）
	float currentJitterY;
	float previousJitterX; // 前フレームのジッター（ピクセル単位）
	float previousJitterY;
};

/// 時間軸型 AA (TAA)。履歴バッファと現在フレームをブレンドして時間的整合性を向上。
class TAAPass : public PostProcessPassBase
{
public:
	TAAPass() : PostProcessPassBase("TAA", "PostProcess_MultiTextureAndCBV")
		, m_frameIndex(0)
		, m_currentJitter(0.0f, 0.0f)
		, m_previousJitter(0.0f, 0.0f)
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
		shaderParams->historyWeight = 0.9f; // 履歴を90%使用
		shaderParams->currentJitterX = m_currentJitter.x;
		shaderParams->currentJitterY = m_currentJitter.y;
		shaderParams->previousJitterX = m_previousJitter.x;
		shaderParams->previousJitterY = m_previousJitter.y;

		cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());
		
		// t0 = current frame
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
		
		// t1 = history buffer
		auto historyRT = context.GetRenderTarget(ConstRenderPref::HistoryBuffer);
		if (historyRT && historyRT->GetSRVHandle())
		{
			cmdList->SetGraphicsRootDescriptorTable(2, historyRT->GetSRVHandle()->gpuHandle);
		}
		
		// t2 = motion vector buffer
		auto motionVectorRT = context.GetRenderTarget(ConstRenderPref::MotionVectorBuffer);
		if (motionVectorRT && motionVectorRT->GetSRVHandle())
		{
			// モーションベクターバッファをSRV状態に遷移（必要な場合）
			if (motionVectorRT->GetCurrentState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			{
				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					motionVectorRT->GetResource(),
					motionVectorRT->GetCurrentState(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				cmdList->ResourceBarrier(1, &barrier);
				motionVectorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
			cmdList->SetGraphicsRootDescriptorTable(3, motionVectorRT->GetSRVHandle()->gpuHandle);
		}
	}

	/// Halton Sequence によるジッター生成（8フレーム周期）
	DirectX::XMFLOAT2 GenerateJitter(int frameIndex)
	{
		// Halton(2, 3) sequence: 品質と収束速度のバランスが良い
		auto halton = [](int index, int base) -> float {
			float result = 0.0f;
			float f = 1.0f;
			int i = index;
			while (i > 0) {
				f /= static_cast<float>(base);
				result += f * static_cast<float>(i % base);
				i = i / base;
			}
			return result;
		};

		// 8サンプルパターン（frameIndex % 8 でループ）
		int sampleIndex = (frameIndex % 8) + 1; // 1-based for Halton
		float u = halton(sampleIndex, 2);
		float v = halton(sampleIndex, 3);

		// [-0.5, 0.5] の範囲に正規化
		return DirectX::XMFLOAT2(u - 0.5f, v - 0.5f);
	}

	/// 次フレームのジッターを計算・更新
	void UpdateJitter()
	{
		m_previousJitter = m_currentJitter;
		m_currentJitter = GenerateJitter(m_frameIndex);
		m_frameIndex++;
	}

	/// 現在のジッターを取得（投影行列に適用するため）
	DirectX::XMFLOAT2 GetCurrentJitter() const { return m_currentJitter; }

private:
	int m_frameIndex;
	DirectX::XMFLOAT2 m_currentJitter;
	DirectX::XMFLOAT2 m_previousJitter;
};
