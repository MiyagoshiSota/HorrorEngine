// G-Buffer 専用ピクセルシェーダー（ライティング・影なし）
// 出力: Albedo, MotionVector, Normal, WorldPos, Material(R,M,AO,Er), Emissive(Eg,Eb)

Texture2D g_AlbedoMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_MetallicRoughnessMap : register(t2);
Texture2D g_EmissiveMap : register(t3);

SamplerState g_Sampler : register(s0);

cbuffer Transform : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float3 CameraPosition;
    float Padding0;
    float4x4 LightViewProj;
    float4x4 PrevViewProj;
    float4x4 CurrViewProj;
}

cbuffer MaterialParams : register(b2)
{
    float4 g_BaseColorFactor;
}

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float4 posLight : TEXCOORD3;
    float4 currPos : TEXCOORD4;
    float4 prevPos : TEXCOORD5;
};

struct PSOutput
{
    float4 albedo : SV_TARGET0;
    float2 motionVector : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float4 worldPos : SV_TARGET3;
    float4 material : SV_TARGET4;   // R=roughness, G=metallic, B=AO, A=emissive.r
    float4 emissiveGB : SV_TARGET5; // R=emissive.g, G=emissive.b, B=0, A=1
};

float3 GetNormalFromMap(float2 uv, float3 worldPos, float3 faceNormal)
{
    float3 tangentNormal = g_NormalMap.Sample(g_Sampler, uv).xyz * 2.0 - 1.0;
    float3 Q1 = ddx(worldPos);
    float3 Q2 = ddy(worldPos);
    float2 st1 = ddx(uv);
    float2 st2 = ddy(uv);
    float3 N = normalize(faceNormal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);
    return normalize(mul(tangentNormal, TBN));
}

PSOutput main(PSInput input)
{
    PSOutput output = (PSOutput)0;

    float4 albedoSample = g_AlbedoMap.Sample(g_Sampler, input.uv);
    output.albedo = float4(albedoSample.rgb * g_BaseColorFactor.rgb, albedoSample.a);

    float2 currNDC = input.currPos.xy / input.currPos.w;
    float2 prevNDC = input.prevPos.xy / input.prevPos.w;
    output.motionVector = float2(currNDC.x - prevNDC.x, prevNDC.y - currNDC.y) * 0.5;

    float3 N = GetNormalFromMap(input.uv, input.worldPos, input.normal);
    output.normal = float4(N * 0.5 + 0.5, 1.0);
    output.worldPos = float4(input.worldPos, 1.0);

    float4 mrSample = g_MetallicRoughnessMap.Sample(g_Sampler, input.uv);
    float ao = max(mrSample.r, 0.001);
    float roughness = mrSample.g;
    float metallic = mrSample.b;
    float3 emissive = g_EmissiveMap.Sample(g_Sampler, input.uv).rgb;

    output.material = float4(roughness, metallic, ao, emissive.r);
    output.emissiveGB = float4(emissive.g, emissive.b, 0.0, 1.0);

    return output;
}
