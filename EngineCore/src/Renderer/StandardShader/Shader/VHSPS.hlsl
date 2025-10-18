// VHSFilter_PS.hlsl

Texture2D g_SceneTexture : register(t0);
SamplerState g_Sampler : register(s0);

// C++側の構造体と一致させる
cbuffer VHSParams : register(b0)
{
    float g_ScanlineIntensity;
    float g_NoiseIntensity;
    float g_Time;
};

// ノイズ生成用のヘルパー関数
float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : SV_TARGET
{
    // 元の色を取得
    float3 finalColor = g_SceneTexture.Sample(g_Sampler, texCoord).rgb;

    // --- 1. スキャンライン効果 ---
    // Y座標に応じて周期的に色を暗くする
    float scanline = sin(texCoord.y * 700.0) * g_ScanlineIntensity;
    finalColor -= scanline;

    // --- 2. ノイズ効果 ---
    // 時間で変化するランダムなノイズを加算
    float noise = (random(texCoord + g_Time) - 0.5) * g_NoiseIntensity;
    finalColor += noise;

    // saturateで色を0.0~1.0の範囲にクランプ
    return float4(saturate(finalColor), 1.0);
}