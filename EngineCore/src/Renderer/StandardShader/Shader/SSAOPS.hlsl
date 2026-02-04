// SSAO: 深度・法線からスクリーン空間AOを計算
// t0=Depth, t1=Normal
// b0=SSAOConstants

Texture2D<float> g_Depth : register(t0);
Texture2D g_Normal : register(t1);

SamplerState g_Sampler : register(s0);

cbuffer SSAOConstants : register(b0)
{
    float4x4 View;
    float4x4 InvProjection;
    float4x4 Projection;
    float4 ProjectionParams; // x=far, y=1/far, z=screenWidth, w=screenHeight
    float Radius;
    float Bias;
    float Power;
    float Padding0;
};

static const int kSampleCount = 16;

// 半球サンプル方向（接空間、正規化済み想定）。シェーダー内でランダム回転する
static const float3 kSampleKernel[kSampleCount] = {
    float3(0.04977, 0.01484, 0.00621),
    float3(-0.04543, -0.06342, 0.03194),
    float3(0.03327, -0.08379, 0.03059),
    float3(-0.06364, 0.02537, 0.03189),
    float3(0.02689, 0.08117, 0.01182),
    float3(-0.08035, -0.03125, 0.02158),
    float3(0.05538, -0.03665, 0.06369),
    float3(-0.02208, 0.09089, 0.02281),
    float3(0.07632, 0.03389, 0.05604),
    float3(-0.03895, -0.06894, 0.05762),
    float3(0.00461, 0.05547, 0.07529),
    float3(-0.09037, -0.02164, 0.04414),
    float3(0.01507, -0.05122, 0.06789),
    float3(-0.05859, 0.06674, 0.04471),
    float3(0.06488, -0.05390, 0.01893),
    float3(-0.03522, 0.03142, 0.08912)
};

// ピクセル座標からランダム回転用のベクトルを生成（ノイズテクスチャなしで簡易）
float3 GetRandomOffset(float2 uv)
{
    float phi = uv.x * 6.28318 + uv.y * 12.0;
    float c = cos(phi);
    float s = sin(phi);
    return float3(c, s, 0.0);
}

// TBNを法線から構築（接空間→ビュー空間）
void BuildTBN(float3 N, out float3 T, out float3 B)
{
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float main(PSInput input) : SV_TARGET
{
    float depth = g_Depth.SampleLevel(g_Sampler, input.uv, 0);
    if (depth <= 0.0) // Reversed-Z: far = 0 (no geometry)
        return 1.0;

    float3 normalEnc = g_Normal.SampleLevel(g_Sampler, input.uv, 0).xyz;
    float3 N = normalEnc * 2.0 - 1.0;
    N = normalize(mul((float3x3)View, N));

    // NDC (DirectX: 0~1 depth, 左手系)
    float2 ndc = input.uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    float4 clipPos = float4(ndc.x, ndc.y, depth, 1.0);
    float4 viewPosH = mul(InvProjection, clipPos);
    float3 viewPos = viewPosH.xyz / viewPosH.w;

    float3 randomVec = GetRandomOffset(input.uv);
    float3 T, B;
    BuildTBN(N, T, B);
    float3x3 TBN = float3x3(T, B, N);
    T = normalize(T + randomVec * 0.1);
    B = normalize(cross(N, T));
    TBN = float3x3(T, B, N);

    float occlusion = 0.0;
    float farPlane = ProjectionParams.x;
    float invScreenWidth = 1.0 / ProjectionParams.z;
    float invScreenHeight = 1.0 / ProjectionParams.w;

    for (int i = 0; i < kSampleCount; i++)
    {
        float3 offset = mul(kSampleKernel[i], TBN);
        float3 sampleViewPos = viewPos + offset * Radius;
        float4 sampleClip = mul(Projection, float4(sampleViewPos, 1.0));
        sampleClip.xyz /= sampleClip.w;

        float2 sampleUV;
        sampleUV.x = sampleClip.x * 0.5 + 0.5;
        sampleUV.y = -sampleClip.y * 0.5 + 0.5;

        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            continue;

        float sampleDepth = g_Depth.SampleLevel(g_Sampler, sampleUV, 0);
        float expectedDepth = sampleClip.z;
        float rangeCheck = smoothstep(0.0, 1.0, Radius / abs(viewPos.z - sampleViewPos.z));
        // Reversed-Z: closer = larger depth. Occlusion when surface at sampleUV is further than sample point.
        if (sampleDepth < expectedDepth - Bias)
            occlusion += rangeCheck;
    }

    occlusion = 1.0 - (occlusion / (float)kSampleCount);
    occlusion = pow(saturate(occlusion), Power);
    return occlusion;
}
