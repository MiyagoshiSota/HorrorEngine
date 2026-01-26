// Skybox Vertex Shader
// キューブマップを使用したSkybox描画用

cbuffer SkyboxCB : register(b0)
{
    float4x4 ViewProj; // View * Projection（回転のみ、平行移動なし）
}

// SharedStruct::Vertexと同じ入力構造
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
    float3 texCoord : TEXCOORD0; // キューブマップサンプリング用の方向ベクトル
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    // 頂点位置をそのまま方向ベクトルとして使用
    output.texCoord = input.pos;

    // ビュー・射影変換
    float4 pos = mul(ViewProj, float4(input.pos, 1.0f));

    // 深度を最大値（1.0）に固定して常に最奥に描画
    // z = w とすることで、除算後に z/w = 1.0 となる
    output.svpos = pos.xyww;

    return output;
}
