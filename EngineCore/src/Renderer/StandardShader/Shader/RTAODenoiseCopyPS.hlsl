// RTAO copy pass (no denoise: RTAORaw -> output)
// t0: AO raw

Texture2D<float> g_AO : register(t0);
SamplerState g_Sampler : register(s0);

struct VSOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(VSOut input) : SV_Target
{
    float ao = g_AO.Sample(g_Sampler, input.uv);
    return float4(ao, 0.0, 0.0, 1.0);
}
