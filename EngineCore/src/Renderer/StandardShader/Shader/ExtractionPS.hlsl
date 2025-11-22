Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

cbuffer BloomParameters : register(b0)
{
	float bloomThreshold;
	float4 padding; // Padding to align to 16 bytes
}

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float4 color = inputTexture.Sample(samplerState, uv);

    // ★安全装置1: NaN (非数) チェック
    // もし計算不能なゴミ値が入っていたら、強制的に黒にする
    if (any(isnan(color.rgb)) || any(isinf(color.rgb)))
    {
        return float4(0, 0, 0, 1);
    }

    // ★安全装置2: 明るさの上限キャップ (Firefly抑制)
    // 100.0 を超えるような超高輝度は 100.0 に抑え込む
    // これをしないと、1点の光が画面全体を白く焼き尽くしてしまいます
    color.rgb = min(color.rgb, 100.0f);
    
    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

    if (brightness > bloomThreshold)
    {
        return color;
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}