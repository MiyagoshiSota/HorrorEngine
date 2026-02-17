cbuffer Transform : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float3 CameraPos;
    float4x4 LightViewProj;
    float Padding0;
}

struct VSInput
{
    float4 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0; 
};

VSOutput main(VSInput input)
{
    VSOutput output;

    float4 localPos = float4(input.position.xyz, 1.0f);
    
    // ワールド変換
    float4 posWorld = mul(localPos, World);

    // ビュー変換
    float4 posView = mul(posWorld, View);

    // プロジェクション変換
    output.position = mul(posView, Proj);

    output.uv = input.uv;

    return output;
}