// =========================================================
// 定数バッファ (C++の構造体とメモリレイアウトを完全に一致させる)
// =========================================================
cbuffer Transform : register(b0)
{
    // Shadow Passでは、ここに「ライトのView行列」と「ライトのProj行列」が入ってきます
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

// =========================================================
// 入力構造体
// =========================================================
struct VSInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
    float4 color : COLOR;
};

// =========================================================
// 出力構造体
// =========================================================
struct VSOutput
{
    // 深度バッファに書き込まれる座標 (Clip Space)
    float4 svpos : SV_POSITION;
    
    // アルファテスト(葉っぱ等)をする場合に必要
    float2 uv : TEXCOORD0;
};

// =========================================================
// Main Entry Point
// =========================================================
VSOutput main(VSInput input)
{
    VSOutput output;

    // 1. ローカル座標の w を 1.0 にして float4 化
    float4 localPos = float4(input.position.xyz, 1.0f);

    // 2. ワールド変換
    float4 posWorld = mul(localPos, World);

    // 3. ビュー・プロジェクション変換
    // C++側で、現在のカスケード用の LightView と LightProj を
    // ここの View, Proj 変数にセットしてくれている前提です。
    float4 posView = mul(posWorld, View);
    output.svpos = mul(posView, Proj);

    // 4. UVパススルー (パンチスルー/AlphaClip用)
    output.uv = input.uv;

    return output;
}