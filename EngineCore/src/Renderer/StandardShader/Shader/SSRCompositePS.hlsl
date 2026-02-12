// SSR 合成: SceneColor + SSRBuffer を roughness に応じてブレンド
// t0=SceneColor, t1=SSRBuffer, t2=Material
// b0=SSRCompositeConstants

Texture2D<float4> g_SceneColor : register(t0);
Texture2D<float4> g_SSRBuffer : register(t1);
Texture2D g_Material : register(t2);

SamplerState g_Sampler : register(s0);

cbuffer SSRCompositeConstants : register(b0)
{
    float ReflectionIntensity;
    float MaxRoughness;
    float Padding[2];
};

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 sceneColor = g_SceneColor.Sample(g_Sampler, input.uv).rgb;
    float4 ssrSample = g_SSRBuffer.Sample(g_Sampler, input.uv);
    float3 reflectionColor = ssrSample.rgb;
    float reflectionMask = ssrSample.a;

    float roughness = g_Material.Sample(g_Sampler, input.uv).r;
    float roughnessFade = 1.0 - smoothstep(MaxRoughness, 1.0, roughness);

    float3 composite = sceneColor + reflectionColor * reflectionMask * roughnessFade * ReflectionIntensity;
    return float4(composite, 1.0);
}
