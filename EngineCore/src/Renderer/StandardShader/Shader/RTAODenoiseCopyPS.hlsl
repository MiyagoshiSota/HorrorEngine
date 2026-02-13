// Copy pass (no denoise, AO / GI 共通)
// t0: signal (AO: .r only, GI: .rgb)

Texture2D<float4> g_Signal : register(t0);
SamplerState g_Sampler : register(s0);

struct VSOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(VSOut input) : SV_Target
{
    float3 signal = g_Signal.Sample(g_Sampler, input.uv).rgb;
    return float4(signal, 1.0);
}
