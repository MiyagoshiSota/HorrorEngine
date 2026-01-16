cbuffer Transform : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float3 CameraPosition;
    float Padding0;

    float4x4 LightViewProj;
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
    float3 worldPos : TEXCOORD1; // ピクセルシェーダーで使うワールド座標
    float3 normal : TEXCOORD2;
    float4 posLight : TEXCOORD3; // ライト視点での座標
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 localPos = float4(input.pos, 1.0f);
    
    // ワールド変換
    float4 worldPos = mul(World, localPos);
    
    // ビュー・射影変換 (カメラ用)
    float4 viewPos = mul(View, worldPos);
    float4 projPos = mul(Proj, viewPos);
    
    output.svpos = projPos;
    output.uv = input.uv;
    
    // 法線の変換 (回転のみ適用するため w=0)
    output.normal = normalize(mul((float3x3) World, input.normal));

    output.worldPos = worldPos.xyz;

    // 単一の LightViewProj 行列を使って、ライト空間へ変換
    // これにより、ピクセルシェーダーでテクスチャのどこを参照すればいいかが分かる
    output.posLight = mul(worldPos, LightViewProj);
    
    return output;
}