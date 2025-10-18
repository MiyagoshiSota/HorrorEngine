Texture2D g_SceneTexture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer MonochromeParams : register(b0)
{
    float intensity;
};

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float4 originalColor = g_SceneTexture.Sample(g_Sampler, uv);
    
    // 輝度を計算 (一般的な計算式)
    float luminance = dot(originalColor.rgb, float3(0.299, 0.587, 0.114));
    float3 monochromeColor = float3(luminance, luminance, luminance);
    
    // intensityを使って元の色とモノクロを線形補間
    float3 finalColor = lerp(originalColor.rgb, monochromeColor, intensity);
    
    return float4(finalColor, originalColor.a);
}