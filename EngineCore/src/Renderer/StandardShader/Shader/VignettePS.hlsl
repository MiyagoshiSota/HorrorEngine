// Vignette_PS.hlsl

Texture2D g_SceneTexture : register(t0); // ジオメトリパスで描画された元絵
SamplerState g_Sampler : register(s0);

cbuffer VignetteParams : register(b0) // Vignette固有のパラメータ用定数バッファ
{
	float g_Intensity; // ビネットの強さ (0.0 - 1.0)
	float g_Smoothness; // ビネットの滑らかさ (0.0 - 1.0)
};

float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET
{
	float4 finalColor = g_SceneTexture.Sample(g_Sampler, texCoord);

	// 画面中央からの距離を計算 (0.0 - 0.5)
	float2 distFromCenter = texCoord - 0.5f;
	float len = length(distFromCenter); // 中心からの距離

	// ビネットの強度を計算
	float vignette = smoothstep(0.5f - g_Smoothness, 0.5f, len);
    
	// ビネットを適用 (色を暗くする)
	finalColor.rgb *= (1.0f - vignette * g_Intensity);

	return finalColor;
}