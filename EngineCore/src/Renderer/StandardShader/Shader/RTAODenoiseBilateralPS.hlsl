// RTAO Bilateral Filter (normal + world position + spatial)
// t0: AO raw, t1: normal, t2: world position
// b0: constants

Texture2D<float> g_AO : register(t0);
Texture2D<float4> g_Normal : register(t1);
Texture2D<float4> g_WorldPos : register(t2);
SamplerState g_Sampler : register(s0);

cbuffer RTAODenoiseConstants : register(b0)
{
    float3 g_CameraPosition;
    float g_DepthSigma;
    float g_NormalSigma;
    float2 g_InvScreenSize;
    float2 g_BlurDirection; // Bilateralでは未使用（Separable用）
    float g_StepSize;       // 未使用（À-Trous用）
    float g_Padding1;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float3 DecodeNormal(float4 enc)
{
    return normalize(enc.xyz * 2.0 - 1.0);
}

float DepthFromWorld(float3 worldPos)
{
    return length(worldPos - g_CameraPosition);
}

float4 main(VSOut input) : SV_Target
{
    float2 uv = input.uv;
    float aoCenter = g_AO.Sample(g_Sampler, uv);
    float3 nCenter = DecodeNormal(g_Normal.Sample(g_Sampler, uv));
    float3 wpCenter = g_WorldPos.Sample(g_Sampler, uv).xyz;
    float depthCenter = DepthFromWorld(wpCenter);

    float sum = 0.0;
    float wsum = 0.0;

    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            float2 duv = float2(x, y) * g_InvScreenSize;
            float2 uv2 = uv + duv;

            float ao = g_AO.Sample(g_Sampler, uv2);
            float3 n = DecodeNormal(g_Normal.Sample(g_Sampler, uv2));
            float3 wp = g_WorldPos.Sample(g_Sampler, uv2).xyz;
            float depth = DepthFromWorld(wp);

            float2 offset = float2(x, y);
            float wSpatial = exp(-dot(offset, offset) * 0.5);

            float nDot = max(0.0, dot(nCenter, n));
            float wNormal = pow(nDot, g_NormalSigma);

            float d = depth - depthCenter;
            float wDepth = exp(-d * d * g_DepthSigma);

            float w = wSpatial * wNormal * wDepth;
            sum += ao * w;
            wsum += w;
        }
    }

    float aoFiltered = (wsum > 1e-5) ? (sum / wsum) : aoCenter;
    return float4(aoFiltered, 0.0, 0.0, 1.0);
}
