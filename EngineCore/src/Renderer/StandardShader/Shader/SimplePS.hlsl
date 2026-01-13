// =========================================================
// テクスチャ & サンプラー定義
// =========================================================
Texture2D g_AlbedoMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_MetallicRoughnessMap : register(t2);
Texture2D g_EmissiveMap : register(t3);
// Texture2D g_shadowMap : register(t4); 
Texture2DArray g_ShadowMap : register(t4); // ShadowMap (Array)

SamplerState g_Sampler : register(s0);
SamplerComparisonState g_ShadowSampler : register(s1);

// 影判定関数 (0.0=影, 1.0=日向)
// 引数を変更: ライト空間座標(float4) と カスケードインデックス(float) を受け取る
float CalculateShadow(float4 posLight, float sliceIndex)
{
    // 1. 射影変換 (w除算)
    // 頂点シェーダから来た座標を正規化します
    float3 projCoords = posLight.xyz / posLight.w;

    // 2. NDC座標(-1~1) を UV座標(0~1) に変換
    // DX12のUVは左上原点、Y下向きなのでYを反転
    projCoords.x = projCoords.x * 0.5 + 0.5;
    projCoords.y = -projCoords.y * 0.5 + 0.5;

    // ライトの範囲外なら影判定しない
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    {
        return 1.0;
    }

    // 3. 深度比較
    float currentDepth = projCoords.z;
    float bias = 0.005; // 調整値

    // ★Texture2DArray用のサンプリング
    // 第2引数は float3(u, v, sliceIndex) になります
    float shadow = g_ShadowMap.SampleCmpLevelZero(
        g_ShadowSampler,
        float3(projCoords.xy, sliceIndex),
        currentDepth - bias
    );

    return shadow;
}

// =========================================================
// 定数バッファ定義
// =========================================================
cbuffer Transform : register(b0)
{
    // Shadow Passでは、ここに「ライトのView行列」と「ライトのProj行列」が入ってきます
    matrix World;
    matrix View;
    matrix Proj;
    float3 CameraPos;
    float Padding0;

    // CSM用の追加データ
    matrix LightViewProj[3];
    float3 SplitDepths;
    int NumCascades;
    float3 Padding1;
}


cbuffer LightParams : register(b1)
{
    float4 AmbientColor;
    int NumDirectionalLights;
    int NumPointLights;
    int NumSpotLights;
    float LightPadding;

    struct DirectionalLight
    {
        float4 Direction;
        float4 ColorAndIntensity;
    } g_DirectionalLights[4];

    struct PointLight
    {
        float4 Position;
        float4 ColorAndIntensity;
        float4 AttenuationAndRange;
    } g_PointLights[28];

    struct SpotLight
    {
        float4 Position;
        float4 Direction;
        float4 ColorAndIntensity;
        float4 SpotAnglesAttenuationAndRange;
    } g_SpotLights[28];
};

cbuffer MaterialParams : register(b2)
{
    float4 g_BaseColorFactor;
}

// =========================================================
// 入力構造体
// =========================================================
struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float4 posLight : TEXCOORD3;
};

// =========================================================
// PBR ヘルパー関数 (変更なし)
// =========================================================
static const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx2 * ggx1;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 GetNormalFromMap(float2 uv, float3 worldPos, float3 faceNormal)
{
    float3 tangentNormal = g_NormalMap.Sample(g_Sampler, uv).xyz * 2.0 - 1.0;

    float3 Q1 = ddx(worldPos);
    float3 Q2 = ddy(worldPos);
    float2 st1 = ddx(uv);
    float2 st2 = ddy(uv);

    float3 N = normalize(faceNormal);
    float3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    float3 B = -normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

// ACESフィルムのトーンマッピング近似関数
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// =========================================================
// メインシェーダー
// =========================================================
float4 main(PSInput input) : SV_TARGET
{
    // 1. テクスチャサンプリング
    float4 albedoSample = g_AlbedoMap.Sample(g_Sampler, input.uv);
    float3 albedo = albedoSample.rgb * g_BaseColorFactor.rgb;

    float4 mrSample = g_MetallicRoughnessMap.Sample(g_Sampler, input.uv);
    float ao = mrSample.r;
    float roughness = mrSample.g;
    float metallic = mrSample.b;

    float3 emissive = g_EmissiveMap.Sample(g_Sampler, input.uv).rgb;

    // 2. 法線計算
    float3 N = GetNormalFromMap(input.uv, input.worldPos, input.normal);
    
    // 視線ベクトル V
    float3 V = normalize(CameraPos - input.worldPos);

    // 3. PBRパラメータ準備
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = float3(0.0, 0.0, 0.0);

    // -----------------------------------------------------
    // Directional Lights
    // -----------------------------------------------------
    for (int i = 0; i < NumDirectionalLights; i++)
    {
        float3 L = normalize(-g_DirectionalLights[i].Direction.xyz);
        float3 H = normalize(V + L);
        
        float3 lightColor = g_DirectionalLights[i].ColorAndIntensity.rgb;
        float intensity = g_DirectionalLights[i].ColorAndIntensity.a;
        float3 radiance = lightColor * intensity;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
            
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;
   
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // -----------------------------------------------------
    // Point Lights
    // -----------------------------------------------------
    for (int j = 0; j < NumPointLights; j++)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - input.worldPos;
        float distance = length(lightVec);
        float range = g_PointLights[j].AttenuationAndRange.y;

        if (distance > range)
            continue;

        float3 L = normalize(lightVec);
        float3 H = normalize(V + L);
        
        float attenuation = 1.0 / (distance * distance + 1.0);
        
        float3 lightColor = g_PointLights[j].ColorAndIntensity.rgb;
        float intensity = g_PointLights[j].ColorAndIntensity.a;
        float3 radiance = lightColor * intensity * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
            
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;
   
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // -----------------------------------------------------
    // Spot Lights
    // -----------------------------------------------------
    for (int k = 0; k < NumSpotLights; k++)
    {
        float3 lightVec = g_SpotLights[k].Position.xyz - input.worldPos;
        float distance = length(lightVec);
        float range = g_SpotLights[k].SpotAnglesAttenuationAndRange.w;

        if (distance > range)
            continue;

        float3 L = normalize(lightVec);
        float3 lightConeDir = normalize(g_SpotLights[k].Direction.xyz);
        
        float3 lightToPixelDir = -L;
        float spotAngleCos = dot(lightToPixelDir, lightConeDir);
        float innerCos = g_SpotLights[k].SpotAnglesAttenuationAndRange.x;
        float outerCos = g_SpotLights[k].SpotAnglesAttenuationAndRange.y;
        float spotFade = smoothstep(outerCos, innerCos, spotAngleCos);

        if (spotFade <= 0.0)
            continue;

        float attenuation = 1.0 / (1.0 + g_SpotLights[k].SpotAnglesAttenuationAndRange.z * distance * distance);
        float3 radiance = g_SpotLights[k].ColorAndIntensity.rgb * g_SpotLights[k].ColorAndIntensity.a * attenuation * spotFade;

        float3 H = normalize(V + L);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
            
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
        
        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;
   
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // 4. 判定 (CSM対応)
    float viewDepth = input.svpos.w;

    // カスケードの選択ロジック (C++の分割距離と合わせる)
    // 0:赤(近), 1:緑(中), 2:青(遠)
    float cascadeIndex = 0.0;
    float3 debugColor = float3(1, 0, 0); // 近景 = 赤

    // ※ split distanceは定数バッファで送るのが正解ですが、デバッグ用に直書きします
    if (viewDepth > 10.0) // 10m以上
    {
        cascadeIndex = 1.0;
        debugColor = float3(0, 1, 0); // 中景 = 緑
    }
    if (viewDepth > 50.0) // 50m以上
    {
        cascadeIndex = 2.0;
        debugColor = float3(0, 0, 1); // 遠景 = 青
    }

    // 影の計算
    float shadowFactor = CalculateShadow(input.posLight, cascadeIndex);

    // ★デバッグ出力: 影の計算結果 × カスケードの色
    // 影になっていれば「暗い色」、日向なら「鮮やかな赤/緑/青」になります
    //return float4(debugColor * shadowFactor, 1.0);

    // -----------------------------------------------------
    // 環境光 & エミッシブ & AO適用
    // -----------------------------------------------------
    float3 ambient = AmbientColor.rgb * albedo * ao;
    float3 color = ambient + Lo * shadowFactor + emissive;
    
    return float4(color, albedoSample.a);
}