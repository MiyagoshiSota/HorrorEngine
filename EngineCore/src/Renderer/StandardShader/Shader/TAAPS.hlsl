// TAA (Temporal Anti-Aliasing) - simplified implementation
// PostProcess_MultiTexture: b0 = TAAParams, t0 = current frame, t1 = history buffer, s0 = sampler

Texture2D g_CurrentFrame : register(t0);
Texture2D g_HistoryBuffer : register(t1);
SamplerState g_Sampler : register(s0);

cbuffer TAAParams : register(b0)
{
	float2 g_InvScreenSize;  // 1/width, 1/height
	float  g_BlendFactor;    // 履歴と現在フレームのブレンド係数 (例: 0.1-0.2)
	float  g_HistoryWeight; // 履歴の重み (例: 0.9-0.95)
};

static const float kLumaR = 0.299;
static const float kLumaG = 0.587;
static const float kLumaB = 0.114;

float GetLuma(float3 rgb)
{
	return dot(rgb, float3(kLumaR, kLumaG, kLumaB));
}

float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	float3 current = g_CurrentFrame.Sample(g_Sampler, texCoord).rgb;
	float3 history = g_HistoryBuffer.Sample(g_Sampler, texCoord).rgb;

	// 簡易的なクランプ（履歴が現在フレームから大きく外れている場合は制限）
	float3 colorMin = current;
	float3 colorMax = current;
	
	// 3x3近傍で現在フレームの最小/最大を計算
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			float2 offset = float2(x, y) * g_InvScreenSize;
			float3 neighbor = g_CurrentFrame.Sample(g_Sampler, texCoord + offset).rgb;
			colorMin = min(colorMin, neighbor);
			colorMax = max(colorMax, neighbor);
		}
	}

	// 履歴をクランプ（現在フレームの近傍範囲内に制限）
	history = clamp(history, colorMin, colorMax);

	// 時間的ブレンド（履歴の重みを高く設定）
	float3 result = lerp(current, history, g_HistoryWeight);

	return float4(result, 1.0);
}
