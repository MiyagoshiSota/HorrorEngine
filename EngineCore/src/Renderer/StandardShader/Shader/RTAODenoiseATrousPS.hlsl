// À-Trous (à-trous) wavelet filter (AO / GI 共通)
// 5x5 kernel with step size 2^j, edge-stopping by depth + normal
// t0: signal (AO: .r only, GI: .rgb), t1: normal, t2: world position
// b0: constants (g_StepSize = 1, 2, 4, 8, 16)

Texture2D<float4> g_Signal : register(t0);
Texture2D<float4> g_Normal : register(t1);
Texture2D<float4> g_WorldPos : register(t2);
SamplerState g_Sampler : register(s0);

cbuffer RTAODenoiseConstants : register(b0)
{
    float3 g_CameraPosition;
    float g_DepthSigma;
    float g_NormalSigma;
    float2 g_InvScreenSize;
    float2 g_BlurDirection;
    float g_StepSize;  // 1, 2, 4, 8, 16
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

// À-Trous 1D カーネル [1, 4, 6, 4, 1] / 16 (インデックス -2..2)
static const float kKernel[5] = { 1.0 / 16.0, 4.0 / 16.0, 6.0 / 16.0, 4.0 / 16.0, 1.0 / 16.0 };

float4 main(VSOut input) : SV_Target
{
    float2 uv = input.uv;
    float3 signalCenter = g_Signal.Sample(g_Sampler, uv).rgb;
    float3 nCenter = DecodeNormal(g_Normal.Sample(g_Sampler, uv));
    float3 wpCenter = g_WorldPos.Sample(g_Sampler, uv).xyz;
    float depthCenter = DepthFromWorld(wpCenter);

    int step = max(1, (int)g_StepSize);
    float3 sumRGB = 0.0;
    float wsum = 0.0;

    [unroll]
    for (int dy = -2; dy <= 2; ++dy)
    {
        [unroll]
        for (int dx = -2; dx <= 2; ++dx)
        {
            float2 duv = float2(dx * step, dy * step) * g_InvScreenSize;
            float2 uv2 = uv + duv;

            float3 signal = g_Signal.Sample(g_Sampler, uv2).rgb;
            float3 n = DecodeNormal(g_Normal.Sample(g_Sampler, uv2));
            float3 wp = g_WorldPos.Sample(g_Sampler, uv2).xyz;
            float depth = DepthFromWorld(wp);

            float wSpatial = kKernel[dx + 2] * kKernel[dy + 2];

            float nDot = max(0.0, dot(nCenter, n));
            float wNormal = pow(nDot, g_NormalSigma);

            float d = depth - depthCenter;
            float wDepth = exp(-d * d * g_DepthSigma);

            float w = wSpatial * wNormal * wDepth;
            sumRGB += signal * w;
            wsum += w;
        }
    }

    float3 filtered = (wsum > 1e-5) ? (sumRGB / wsum) : signalCenter;
    return float4(filtered, 1.0);
}
