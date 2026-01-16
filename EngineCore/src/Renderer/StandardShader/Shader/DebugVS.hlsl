cbuffer DebugCOn : register(b0)
{
	float4x4 WorldViewProj;
};

struct VSInput
{
	float3 PositionOS : POSITION;
};

struct PSInput
{
	float4 PositionCS : SV_POSITION;
};

PSInput main(VSInput input)
{
	PSInput output = (PSInput)0;

	output.PositionCS = mul(float4(input.PositionOS, 1.0f), WorldViewProj);
	
	return output;
}