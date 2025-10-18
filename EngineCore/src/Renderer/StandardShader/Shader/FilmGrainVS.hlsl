// FilmGrain_VS.hlsl

struct VS_INPUT
{
    uint VertexID : SV_VertexID;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // 画面全体を覆う三角形の頂点座標とUV座標を生成
    output.TexCoord = float2((input.VertexID << 1) & 2, input.VertexID & 2);
    output.Position = float4(output.TexCoord * 2.0f - 1.0f, 0.0f, 1.0f);
    
    // DirectXはYが下向きなのでUVを反転
    output.Position.y = -output.Position.y;
    
    return output;
}