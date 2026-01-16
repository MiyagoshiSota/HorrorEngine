// =========================================================
// 定数バッファ (C++の SharedStruct::Transform と一致させる)
// =========================================================
cbuffer Transform : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float3 CameraPos;
    float4x4 LightViewProj;
    float Padding0;
}

// =========================================================
// 入力・出力構造体
// =========================================================
struct VSInput
{
    float4 position : POSITION;
    float2 uv : TEXCOORD0; // アルファテスト(葉っぱ等)をするなら必要
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct VSOutput
{
    float4 position : SV_POSITION; // ★最重要: これがDepthBufferに書き込まれる
    float2 uv : TEXCOORD0; // PSでテクスチャを読むなら渡す
};

VSOutput main(VSInput input)
{
    VSOutput output;

    // 1. w=1.0 を保証
    float4 localPos = float4(input.position.xyz, 1.0f);
    
    // 2. ワールド変換
    float4 posWorld = mul(localPos, World);

    // 3. ビュー変換
    float4 posView = mul(posWorld, View);

    // 4. プロジェクション変換
    output.position = mul(posView, Proj);

    // テクスチャ対応が必要なら
    output.uv = input.uv;

    return output;
}