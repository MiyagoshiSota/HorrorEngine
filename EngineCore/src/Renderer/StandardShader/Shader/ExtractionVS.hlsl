 struct VSOutput
{
    float4 position : SV_POSITION; // ピクセルのスクリーン座標
    float2 uv : TEXCOORD; // テクスチャをサンプリングするためのUV座標
};

VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;

    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.uv * 2.0f - 1.0f, 0.0f, 1.0f);
    
    // スクリーン座標とテクスチャ座標のY軸の向きを合わせる
    output.position.y = -output.position.y;

    return output;
}