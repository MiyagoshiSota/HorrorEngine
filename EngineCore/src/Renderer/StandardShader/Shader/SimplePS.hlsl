struct Light
{
    float4 Direction; // ライトの方向
    float4 Color; // ライトの色 (wは強度)
    float4 Attenuation; // 減衰係数 (未使用)
};

cbuffer LightParams : register(b1)
{
    float4 AmbientColor; // 環境光の色
    int NumLights; // ライトの数 (未使用)
    Light g_DirectionalLight; // とりあえずライトは1つ
};

Texture2D g_MainTex : register(t0);
SamplerState g_Sampler : register(s0);

// 頂点シェーダーからの入力 (VSOutputと完全に一致させる)
struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

float4 main(PSInput input) : SV_TARGET
{
    // テクスチャから基本色（アルベド）を取得
    float3 albedo = g_MainTex.Sample(g_Sampler, input.uv).rgb;
    
    // 法線を正規化 (補間によって長さが1でなくなる可能性があるため)
    float3 normal = normalize(input.normal);

    // 1. 環境光を計算
    float3 finalColor = albedo * AmbientColor.rgb;
    
    // 2. 平行光源の拡散反射光（Diffuse）を計算
    // ライトの方向ベクトル（光源へ向かうベクトル）
    float3 lightDir = normalize(-g_DirectionalLight.Direction.xyz);
    
    // 法線とライト方向の内積を取り、光の当たり具合を計算 (0以上を保証)
    float diffuseFactor = saturate(dot(normal, lightDir));
    
    // ライトの色と強度を乗算
    float3 diffuseColor = g_DirectionalLight.Color.rgb * g_DirectionalLight.Color.a * diffuseFactor;

    // 3. 最終的な色に拡散反射光を加算
    finalColor += albedo * diffuseColor;
    
    // 最終的な色を返す (saturateで0-1の範囲にクランプ)
    return float4(saturate(finalColor), 1.0f);
}