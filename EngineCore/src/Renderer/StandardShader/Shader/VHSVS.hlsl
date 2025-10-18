struct VS_OUTPUT
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;

    // 画面全体を覆う四角形の頂点位置を計算
    output.TexCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.Position = float4(output.TexCoord * 2.0f - 1.0f, 0.0f, 1.0f);

    // UV反転
    output.Position.y = -output.Position.y;

    return output;
}
