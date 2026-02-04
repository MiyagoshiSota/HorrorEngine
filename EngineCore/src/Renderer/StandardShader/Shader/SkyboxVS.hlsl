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

    output.texCoord = input.pos;

    // ベクトルを行列の左から掛ける
    float4 pos = mul(float4(input.pos, 1.0f), ViewProj);

    // NOTE: Reverse-Z対応 深度を0.0(最遠)に固定する
    output.svpos = pos.xyzw;
    output.svpos.z = 0.0f; 

    return output;
}
