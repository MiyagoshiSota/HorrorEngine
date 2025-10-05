Texture2D g_InputTexture : register(t0);
SamplerState g_Sampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION; // 必須：ピクセルのスクリーン座標
    float2 texcoord : TEXCOORD; // テクスチャをサンプリングするためのUV座標
};

float4 main(VSOutput input) : SV_TARGET
{
    // 1. 入力テクスチャから、対応するUV座標のピクセル色を取得
    float4 color = g_InputTexture.Sample(g_Sampler, input.texcoord);

    // 2. 取得した色をグレースケールに変換
    //    人間の目の感度を考慮した比率(NTSC係数)でRGBを合成するのが一般的
    float grayscale = dot(color.rgb, float3(0.299, 0.587, 0.114));
    
    // 3. RGBをすべて同じ値(grayscale)にし、アルファ値は元のままにして出力
    return float4(grayscale, grayscale, grayscale, color.a);
}