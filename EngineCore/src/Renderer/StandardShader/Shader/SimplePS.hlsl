// ジオメトリパスからの入力テクスチャ（アルベド）
Texture2D    g_MainTex   : register(t0);
SamplerState g_Sampler   : register(s0);

// ライトの情報を格納する定数バッファ
cbuffer LightParams : register(b1)
{
    float4 AmbientColor;
    int    NumDirectionalLights;
    int    NumPointLights;
    float2 Padding; // アライメント調整

    // 平行光源のデータ配列
    struct DirectionalLight
    {
        float4 Direction;
        float4 ColorAndIntensity;
    } g_DirectionalLights[4]; // 最大4つ

    // ポイントライトのデータ配列
    struct PointLight
    {
        float4 Position;
        float4 ColorAndIntensity;
        float4 AttenuationAndRange; // x:減衰率, y:影響範囲
    } g_PointLights[28]; // 最大28個
};

cbuffer DeffuseParams : register(b2)
{
    float4 DiffuseColor;
}

// 頂点シェーダーから渡されるデータ
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
    
    // 補間された法線を正規化
    float3 normal = normalize(input.normal);

    // 環境光で最終的な色を初期化
    float3 finalColor = albedo * AmbientColor.rgb;
    
    // 平行光源の計算
    for (int i = 0; i < NumDirectionalLights; i++)
    {
        // ライトの方向ベクトル（光源へ向かうベクトル）
        float3 lightDir = normalize(-g_DirectionalLights[i].Direction.xyz);
        
        // 拡散反射係数
        float diffuseFactor = saturate(dot(normal, lightDir));
        
        // ライトの色と強度
        float3 lightColor = g_DirectionalLights[i].ColorAndIntensity.rgb;
        float lightIntensity = g_DirectionalLights[i].ColorAndIntensity.a;

        // 拡散反射光を加算
        finalColor += albedo * lightColor * lightIntensity * diffuseFactor;
    }

    // ポイントライトの計算
    for (int j = 0; j < NumPointLights; j++)
    {
        // ピクセル位置からライトへのベクトル
        float3 lightVec = g_PointLights[j].Position.xyz - input.worldPos;
        float dist = length(lightVec);
        float range = g_PointLights[j].AttenuationAndRange.y;

        // ライトの影響範囲外なら処理をスキップ
        if (dist > range)
        {
            continue;
        }

        // ライトの方向ベクトル
        float3 lightDir = normalize(lightVec);
        
        // 距離による減衰
        float attenuation = 1.0 / (1.0 + g_PointLights[j].AttenuationAndRange.x * dist * dist);
        
        // 拡散反射係数
        float diffuseFactor = saturate(dot(normal, lightDir));

        // ライトの色と強度
        float3 lightColor = g_PointLights[j].ColorAndIntensity.rgb;
        float lightIntensity = g_PointLights[j].ColorAndIntensity.a;

        // 拡散反射光（減衰を考慮）を加算
        finalColor += albedo * lightColor * lightIntensity * diffuseFactor * attenuation;
        
        // return float4(attenuation,0.0f,0.0f, 1.0f);
    }
    
    // 最終的な色を 0.0 ~ 1.0 の範囲に収めて返す
    return float4(saturate(finalColor * DiffuseColor), 1.0f);
}