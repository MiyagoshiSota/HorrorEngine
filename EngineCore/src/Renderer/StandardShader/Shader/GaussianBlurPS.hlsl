Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

cbuffer BlurParameters : register(b0)
{
    float2 g_Direction; // ブラーの方向 (例: (1,0) は水平、(0,1) は垂直)
    float g_TextureWidth; // テクスチャの幅
    float g_TextureHeight; // テクスチャの高さ
}

// ガウス重み係数 (5サンプル分)
static const float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };


float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float2 tex_offset = float2(1.0 / g_TextureWidth, 1.0 / g_TextureHeight);

    // 中心ピクセル (Weight[0])
    float3 result = inputTexture.Sample(samplerState, uv).rgb * weights[0];
    
    // 周辺ピクセル
    for (int i = 1; i < 5; ++i)
    {
        float2 offsetVector = g_Direction * tex_offset * i;

        float2 offsetPos = uv + offsetVector; 
        float2 offsetNeg = uv - offsetVector; 

        result += inputTexture.Sample(samplerState, offsetPos).rgb * weights[i];
        result += inputTexture.Sample(samplerState, offsetNeg).rgb * weights[i];
    }

    return float4(result, 1.0);
}