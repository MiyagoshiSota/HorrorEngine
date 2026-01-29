// FXAA (Fast Approximate Anti-Aliasing) - simplified single-pass
// PostProcess_TextureAndCBV: b0 = FXAAParams, t0 = scene texture, s0 = sampler

Texture2D g_SceneTexture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer FXAAParams : register(b0)
{
	float2 g_InvScreenSize;  // 1/width, 1/height
	float  g_EdgeThreshold; // エッジ検出の閾値 
	float  g_EdgeThresholdMin; // 最小閾値
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
	float3 nw = g_SceneTexture.Sample(g_Sampler, texCoord + float2(-1.0, -1.0) * g_InvScreenSize).rgb;
	float3 ne = g_SceneTexture.Sample(g_Sampler, texCoord + float2( 1.0, -1.0) * g_InvScreenSize).rgb;
	float3 sw = g_SceneTexture.Sample(g_Sampler, texCoord + float2(-1.0,  1.0) * g_InvScreenSize).rgb;
	float3 se = g_SceneTexture.Sample(g_Sampler, texCoord + float2( 1.0,  1.0) * g_InvScreenSize).rgb;
	float3 m  = g_SceneTexture.Sample(g_Sampler, texCoord).rgb;

	float lumaNw = GetLuma(nw);
	float lumaNe = GetLuma(ne);
	float lumaSw = GetLuma(sw);
	float lumaSe = GetLuma(se);
	float lumaM  = GetLuma(m);

	float lumaMin = min(lumaM, min(min(lumaNw, lumaNe), min(lumaSw, lumaSe)));
	float lumaMax = max(lumaM, max(max(lumaNw, lumaNe), max(lumaSw, lumaSe)));
	float lumaRange = lumaMax - lumaMin;

	// コントラストが低い領域はスキップ
	if (lumaRange < max(g_EdgeThresholdMin, lumaMax * g_EdgeThreshold))
		return float4(m, 1.0);

	// エッジに垂直方向のブレンド
	float2 dir;
	dir.x = -((lumaNw + lumaNe) - (lumaSw + lumaSe));
	dir.y =  ((lumaNw + lumaSw) - (lumaNe + lumaSe));
	float dirReduce = max((lumaNw + lumaNe + lumaSw + lumaSe) * (0.25 * 0.25), g_EdgeThresholdMin);
	float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	dir = min(float2(8.0, 8.0), max(float2(-8.0, -8.0), dir * rcpDirMin)) * g_InvScreenSize;

	float3 resultA = 0.5 * (
		g_SceneTexture.Sample(g_Sampler, texCoord + dir * (1.0/3.0 - 0.5)).rgb +
		g_SceneTexture.Sample(g_Sampler, texCoord + dir * (2.0/3.0 - 0.5)).rgb);
	float3 resultB = resultA * 0.5 + 0.25 * (
		g_SceneTexture.Sample(g_Sampler, texCoord + dir * (0.0 - 0.5)).rgb +
		g_SceneTexture.Sample(g_Sampler, texCoord + dir * (1.0 - 0.5)).rgb);

	float lumaB = GetLuma(resultB);
	if (lumaB < lumaMin || lumaB > lumaMax)
		return float4(resultA, 1.0);
	return float4(resultB, 1.0);
}
