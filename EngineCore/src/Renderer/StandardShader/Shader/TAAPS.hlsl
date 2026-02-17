// TAA (Temporal Anti-Aliasing) with Motion Vector Support
// b0 = TAAParams, t0 = current frame, t1 = history buffer, t2 = motion vector, s0 = sampler

Texture2D g_CurrentFrame : register(t0);
Texture2D g_HistoryBuffer : register(t1);
Texture2D g_MotionVectorBuffer : register(t2);
SamplerState g_Sampler : register(s0);

cbuffer TAAParams : register(b0)
{
	float2 g_InvScreenSize;   // 1/width, 1/height
	float  g_BlendFactor;     // 履歴と現在フレームのブレンド係数
	float  g_HistoryWeight;   // 履歴の重み
	float2 g_CurrentJitter;   // 現在フレームのジッター（px）
};

float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	float3 current = g_CurrentFrame.Sample(g_Sampler, texCoord).rgb;
	
	// モーションベクターを読み取り
	float2 motionVector = g_MotionVectorBuffer.Sample(g_Sampler, texCoord).rg;
	
	// 履歴バッファのサンプリング位置
	float2 historyUV = texCoord - motionVector;
	
	// 履歴バッファが画面外を参照する場合は現在フレームを使用
	if (historyUV.x < 0.0 || historyUV.x > 1.0 || historyUV.y < 0.0 || historyUV.y > 1.0)
	{
		return float4(current, 1.0);
	}
	
	float3 history = g_HistoryBuffer.Sample(g_Sampler, historyUV).rgb;

	// 3x3近傍で現在フレームの最小/最大を計算
	float3 colorMin = current;
	float3 colorMax = current;
	float3 colorSum = current;
	
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			if (x == 0 && y == 0) continue; // 中心ピクセルは既に取得済み
			
			float2 offset = float2(x, y) * g_InvScreenSize;
			float3 neighbor = g_CurrentFrame.Sample(g_Sampler, texCoord + offset).rgb;
			colorMin = min(colorMin, neighbor);
			colorMax = max(colorMax, neighbor);
			colorSum += neighbor;
		}
	}
	
	// 履歴を現在フレームの近傍範囲内に制限
	history = clamp(history, colorMin, colorMax);

	// 時間的ブレンド
	float3 result = lerp(current, history, g_HistoryWeight);

	return float4(result, 1.0);
}
