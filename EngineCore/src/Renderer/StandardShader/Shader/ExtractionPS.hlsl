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

    // NaN or Inf check
    if (any(isnan(color.rgb)) || any(isinf(color.rgb)))
    {
        return float4(0, 0, 0, 1);
    }

    // Brightness limit
    color.rgb = min(color.rgb, 100.0f);
    
    // Brightness calculation
    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

    // Brightness threshold
    if (brightness > bloomThreshold)
    {
        return color;
    }
    else
    {
        return float4(0, 0, 0, 1);
    }
}