
cbuffer Transform : register(b0)
{
    matrix World;
    matrix View;
    matrix Proj;
    float3 CameraPosition;
    float Padding0;

    // CSM用の追加データ
    matrix LightViewProj[3];
    float3 SplitDepths;
    int NumCascades;
    float3 Padding1;
}

// 入力構造体
struct VSInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
    float4 color : COLOR;
};

// 出力構造体
struct VSOutput
{
    // 深度バッファに書き込まれる座標 (Clip Space)
    float4 svpos : SV_POSITION;
    
    float2 uv : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // ローカル座標の w を 1.0 にして float4 化
    float4 localPos = float4(input.position.xyz, 1.0f);

    // ワールド変換
    float4 posWorld = mul(localPos, World);

    // View・Proj変換
    float4 posView = mul(posWorld, View);
    output.svpos = mul(posView, Proj);

    // UVパススルー
    output.uv = input.uv;

    return output;
}