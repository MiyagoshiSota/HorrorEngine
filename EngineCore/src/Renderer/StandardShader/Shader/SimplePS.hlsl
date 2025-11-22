// =========================================================
// テクスチャ & サンプラー定義
// =========================================================
Texture2D g_AlbedoMap : register(t0); // BaseColor
Texture2D g_NormalMap : register(t1); // Normal
Texture2D g_MetallicRoughnessMap : register(t2); // Metallic(B) + Roughness(G) (+ AO(R)?)
Texture2D g_EmissiveMap : register(t3); // Emissive
TextureCube g_SpecularIBLMap : register(t4);

SamplerState g_Sampler : register(s0);

// =========================================================
// 定数バッファ定義
// =========================================================

cbuffer Transform : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Proj;
    float3 CameraPos; // ★追加: 正しいスペキュラ計算に必須
    float Padding0;
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
    // 法線マップがDirectX形式かOpenGL形式か確認が必要。ここではそのまま使用。
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

    // ★修正箇所: MetallicRoughnessMapの分離
    // 一般的なglTF (ORM) 形式を想定:
    // R = Occlusion (AO)
    // G = Roughness
    // B = Metallic
    float4 mrSample = g_MetallicRoughnessMap.Sample(g_Sampler, input.uv);

    float ao = mrSample.r; // AOが含まれていない場合は 1.0 とする
    float roughness = mrSample.g; // Roughness
    float metallic = mrSample.b; // Metallic

    // もしAOがテクスチャに含まれていない(入力リストにない)場合は以下を有効化
    // float ao = 1.0; 

    float3 emissive = g_EmissiveMap.Sample(g_Sampler, input.uv).rgb;

    // 2. 法線計算
    float3 N = GetNormalFromMap(input.uv, input.worldPos, input.normal);
    
    // ★修正箇所: 視線ベクトル V の計算
    // ピクセル位置からカメラ位置へのベクトルが必要です
    float3 V = normalize(CameraPos - input.worldPos);

    // 3. PBRパラメータ準備
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    float3 Lo = float3(0.0, 0.0, 0.0);

    // -----------------------------------------------------
    // Directional Lights
    // Cook-Torrance BRDF
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
        
        // 減衰
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

    // -----------------------------------------------------
    // 環境光 & エミッシブ & AO適用
    // -----------------------------------------------------
    // AOは一般的に環境光（Ambient）にのみ乗算します
	float3 ambient = AmbientColor.rgb * albedo * ao;
    float3 color = ambient + Lo + emissive;

    // 1. ACESトーンマッピングを適用 (HDR -> LDR)
    //color = ACESFilm(color);

    // 2. ガンマ補正 (Linear -> sRGB)
    // これをしないと暗部が極端に黒く潰れます
    //color = pow(color, 1.0 / 2.2);

    //return float4(color, albedoSample.a);
		
    return float4(color, albedoSample.a);
}