/* cbuffer Transform : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float3 CameraPos; // ★追加: 正しいスペキュラ計算に必須
    float4x4 LightViewProj;
    float Padding0;
}*/

cbuffer Transform : register(b0)
{
    matrix World;
    matrix View;
    matrix Proj;
    float3 CameraPosition;
    float Padding0;

    matrix LightViewProj[3];
    float3 SplitDepths;
    int NumCascades;
    float3 Padding1;
}


struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float4 posLight : TEXCOORD3;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput) 0;

	float4 localPos = float4(input.pos, 1.0f);
	float4 worldPos = mul(World, localPos);
	float4 viewPos = mul(View, worldPos);
	float4 projPos = mul(Proj, viewPos);
	output.svpos = projPos;
	output.uv = input.uv;
	output.normal = input.normal;
	output.worldPos = worldPos.xyz;
    output.posLight = mul(worldPos, LightViewProj[NumCascades]);
	
    return output;
}