struct PS_INPUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_Target
{
    float4 color = float4(0.5,0.5,0.5,0.1); // 雨の色と透明度
    return color;
}