// Skybox Pixel Shader
// キューブマップを使用したSkybox描画用

TextureCube g_Skybox : register(t0);
SamplerState g_Sampler : register(s0);

cbuffer SkyboxCB : register(b0)
{
    float4x4 ViewProj;  // VS 用（PS では未使用）
    float3 Tint;
    float Intensity;
}

struct PSInput
{
    float4 svpos : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    // 方向ベクトルでキューブマップをサンプリング
    float3 color = g_Skybox.Sample(g_Sampler, input.texCoord).rgb;
    color = color * Tint * Intensity;

    return float4(color, 1.0f);
}
