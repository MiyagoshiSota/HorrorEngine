// ChromaticAberration_PS.hlsl

Texture2D g_SceneTexture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer ChromaticAberrationParams : register(b0)
{
    float g_Offset; // 色ずれの強さ
};

float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET
{
    // Rチャンネルを少しずらしてサンプリング
    float r = g_SceneTexture.Sample(g_Sampler, texCoord + float2(g_Offset, 0.0)).r;
    
    // Gチャンネルはそのままサンプリング
    float g = g_SceneTexture.Sample(g_Sampler, texCoord).g;

    // Bチャンネルを逆方向に少しずらしてサンプリング
    float b = g_SceneTexture.Sample(g_Sampler, texCoord - float2(g_Offset, 0.0)).b;

    // ずらしたRGBを合成して最終的な色とする
    return float4(r, g, b, 1.0);
}