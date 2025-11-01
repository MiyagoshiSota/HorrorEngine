struct PSInput
{
	float4 PositionCS : SV_POSITION;
};

float4 main(PSInput input) : SV_TARGET
{
	return float4(1,1,0,1);
}