// ライティングパス：G-Buffer + シャドウから最終カラーを計算
// t0=Albedo, t1=Normal, t2=WorldPos, t3=Motion, t4=Material, t5=Emissive, t6=ShadowMap
// b0=LightingTransform, b1=LightParams

Texture2D g_Albedo : register(t0);
Texture2D g_Normal : register(t1);
Texture2D g_WorldPos : register(t2);
Texture2D g_Material : register(t4);
Texture2D g_Emissive : register(t5);
Texture2D g_ShadowMap : register(t6);

SamplerState g_Sampler : register(s0);
SamplerComparisonState g_ShadowSampler : register(s1);

cbuffer LightingTransform : register(b0)
{
    float3 CameraPosition;
    float Padding0;
    float4x4 LightViewProj;
    int ShadowMode;  // 0=None, 1=RasterDepth, 2=RayTracedMask, 3=RayTracedVisibility
    int Padding1[3];
}

cbuffer LightParams : register(b1)
{
    float4 AmbientColor;
    int NumDirectionalLights;
    int NumPointLights;
    int NumSpotLights;
    float LightPadding;

    struct DirectionalLight
    {
        float4 Direction;
        float4 ColorAndIntensity;
    } g_DirectionalLights[4];

    struct PointLight
    {
        float4 Position;
        float4 ColorAndIntensity;
        float4 AttenuationAndRange;
    } g_PointLights[28];

    struct SpotLight
    {
        float4 Position;
        float4 Direction;
        float4 ColorAndIntensity;
        float4 SpotAnglesAttenuationAndRange;
    } g_SpotLights[28];
};

static const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float CalculateShadow(float4 posLight)
{
    if (ShadowMode == 0) return 1.0;

    float3 projCoords = posLight.xyz / posLight.w;
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = -projCoords.y * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 1.0;

    if (ShadowMode == 2)
        return g_ShadowMap.Sample(g_Sampler, projCoords.xy).r;

    float currentDepth = projCoords.z;
    float bias = 0.005;
    float shadow = g_ShadowMap.SampleCmpLevelZero(g_ShadowSampler, projCoords.xy, currentDepth - bias);
    return 1.0 - shadow;
}

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 albedoSample = g_Albedo.Sample(g_Sampler, input.uv);
    float3 albedo = albedoSample.rgb;
    float3 N = g_Normal.Sample(g_Sampler, input.uv).xyz * 2.0 - 1.0;
    float3 worldPos = g_WorldPos.Sample(g_Sampler, input.uv).xyz;

    // G-Buffer Material: R=roughness, G=metallic, B=AO, A=emissive.r
    // G-Buffer Emissive: R=emissive.g, G=emissive.b
    float4 matSample = g_Material.Sample(g_Sampler, input.uv);
    float roughness = matSample.r;
    float metallic = matSample.g;
    float ao = max(matSample.b, 1.0);
    float2 emissiveGB = g_Emissive.Sample(g_Sampler, input.uv).rg;
    float3 emissive = float3(matSample.a, emissiveGB.r, emissiveGB.g);

    float3 V = normalize(CameraPosition - worldPos);
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = float3(0.0, 0.0, 0.0);

    // Directional Lights
    for (int i = 0; i < NumDirectionalLights; i++)
    {
        float3 L = normalize(-g_DirectionalLights[i].Direction.xyz);
        float3 H = normalize(V + L);
        float3 radiance = g_DirectionalLights[i].ColorAndIntensity.rgb * g_DirectionalLights[i].ColorAndIntensity.a;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        float3 kS = F;
        float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Point Lights
    for (int j = 0; j < NumPointLights; j++)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - worldPos;
        float distance = length(lightVec);
        if (distance > g_PointLights[j].AttenuationAndRange.y) continue;
        float3 L = normalize(lightVec);
        float3 H = normalize(V + L);
        float attenuation = 1.0 / (distance * distance + 1.0);
        float3 radiance = g_PointLights[j].ColorAndIntensity.rgb * g_PointLights[j].ColorAndIntensity.a * attenuation;
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        float3 kS = F;
        float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Spot Lights
    for (int k = 0; k < NumSpotLights; k++)
    {
        float3 lightVec = g_SpotLights[k].Position.xyz - worldPos;
        float distance = length(lightVec);
        if (distance > g_SpotLights[k].SpotAnglesAttenuationAndRange.w) continue;
        float3 L = normalize(lightVec);
        float3 lightConeDir = normalize(g_SpotLights[k].Direction.xyz);
        float spotAngleCos = dot(-L, lightConeDir);
        float spotFade = smoothstep(g_SpotLights[k].SpotAnglesAttenuationAndRange.y, g_SpotLights[k].SpotAnglesAttenuationAndRange.x, spotAngleCos);
        if (spotFade <= 0.0) continue;
        float attenuation = 1.0 / (1.0 + g_SpotLights[k].SpotAnglesAttenuationAndRange.z * distance * distance);
        float3 radiance = g_SpotLights[k].ColorAndIntensity.rgb * g_SpotLights[k].ColorAndIntensity.a * attenuation * spotFade;
        float3 H = normalize(V + L);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        float3 kS = F;
        float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    float4 posLight = mul(float4(worldPos, 1.0), LightViewProj);
    float shadowFactor = CalculateShadow(posLight);
    float3 ambient = AmbientColor.rgb * albedo * ao;
    float3 color = ambient + Lo * shadowFactor * ao + emissive;
    return float4(color, albedoSample.a);
}
