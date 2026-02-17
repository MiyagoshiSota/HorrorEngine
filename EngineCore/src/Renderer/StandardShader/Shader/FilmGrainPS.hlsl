// FilmGrain_PS.hlsl

Texture2D g_SceneTexture : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer FilmGrainParams : register(b0)
{
    float g_Intensity; // ノイズの強さ (0.0 - 1.0)
    float g_Time; // 時間 
    float2 g_ScreenSize; // 画面サイズ 
};

// ノイズ生成用の関数
float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123);
}

float noise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    float a = random(i);
    float b = random(i + float2(1.0, 0.0));
    float c = random(i + float2(0.0, 1.0));
    float d = random(i + float2(1.0, 1.0));
    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}


float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET
{
    float4 finalColor = g_SceneTexture.Sample(g_Sampler, texCoord);

    // ノイズのUV座標を調整
    float2 noiseUV = texCoord * g_ScreenSize / 100.0f; // ノイズのスケールを調整
    noiseUV += g_Time * 0.1f; // 時間でノイズをスクロールさせる

    // ノイズ値を生成
    float grain = noise(noiseUV) - 0.5f; 

    // ノイズを適用
    finalColor.rgb += grain * g_Intensity;

    return finalColor;
}