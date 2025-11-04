// ジオメトリパスからの入力テクスチャ（アルベド）
Texture2D    g_MainTex   : register(t0);
SamplerState g_Sampler   : register(s0);

// ライトの情報を格納する定数バッファ
cbuffer LightParams : register(b1)
{
    float4 AmbientColor;
    int NumDirectionalLights;
    int NumPointLights;
    int NumSpotLights;
    float Padding; // アライメント調整

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

    // スポットライトのデータ配列
    struct SpotLight
    {
        float4 Position;
        float4 Direction;
        float4 ColorAndIntensity;
        float4 SpotAnglesAttenuationAndRange; // x: InnerAngle, y: OuterAngle
    } g_SpotLights[28]; // 最大4つ
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

    // スポットライトの計算
    for (int k = 0; k < NumSpotLights; k++)
    {
        // ライトのプロパティを取得
        float3 lightPos = g_SpotLights[k].Position.xyz;
        float3 lightColor = g_SpotLights[k].ColorAndIntensity.rgb;
        float lightIntensity = g_SpotLights[k].ColorAndIntensity.a;
        
        // ライトの「コーン」が向いている中心方向 (正規化されていること)
        float3 lightConeDir = normalize(g_SpotLights[k].Direction.xyz);
        
        // CPU側で pre-calculate された cos(角度) が格納されていると仮定
        // (HLSL側でcos()を計算するより高速)
        float innerConeCos = g_SpotLights[k].SpotAnglesAttenuationAndRange.x; //例: cos(10度) = 0.984
        float outerConeCos = g_SpotLights[k].SpotAnglesAttenuationAndRange.y; //例: cos(20度) = 0.939

        // ポイントライトとしての基本計算
        
        // ピクセルからライトへのベクトル
        float3 lightVec = lightPos - input.worldPos;
        // ピクセルからライトへの距離
        float dist = length(lightVec);
        // ピクセルからライトへの「方向」ベクトル (拡散反射用)
        float3 lightDir = normalize(lightVec);

        // 拡散反射の計算
        // (法線とライト方向の内積)
        float diffuseFactor = saturate(dot(normal, lightDir));

        // ピクセルがライトと逆方向を向いていたら、光は当たらない
        if (diffuseFactor <= 0.0)
        {
            continue;
        }

        // スポットライトのコーン減衰
        // 「ライトからピクセルへ」の方向ベクトル (コーン判定用)
        float3 lightToPixelDir = -lightDir;
        
        // ライトのコーン中心方向と、ピクセルへの方向の内積
        // (この値が 1.0 に近いほど、ピクセルはコーンの中心にいる)
        float spotAngleCos = dot(lightToPixelDir, lightConeDir);
        
        // コーンの減衰係数 (0.0 から 1.0)
        // smoothstep(edge0, edge1, x) を利用
        // x (spotAngleCos) が...
        //   outerConeCos (例:0.939) より小さい場合 = 0.0 (コーンの外)
        //   innerConeCos (例:0.984) より大きい場合 = 1.0 (コーンの内)
        //   その間の場合 = 0.0から1.0へ滑らかに遷移
        float spotFade = smoothstep(outerConeCos, innerConeCos, spotAngleCos);
        
        // コーンの外側なら、光は当たらない
        if (spotFade <= 0.0)
        {
            continue;
        }
        
        // 距離による減衰の計算
        float range = g_SpotLights[k].SpotAnglesAttenuationAndRange.w;
        
        // ライトの影響範囲外なら処理をスキップ
        if (dist > range)
        {
            continue;
        }
        
        // 距離による減衰
        float attenuation = 1.0 / (1.0 + g_SpotLights[k].SpotAnglesAttenuationAndRange.z * dist * dist); 

        // 最終的な色を加算
        // finalColor += [マテリアルの色] * [ライトの色] * [ライトの強さ] * [拡散反射係数] * [スポット減衰] * [距離減衰];
        finalColor += albedo * lightColor * lightIntensity * diffuseFactor * spotFade * attenuation;
    }
    
    // 最終的な色を 0.0 ~ 1.0 の範囲に収めて返す
    return float4(saturate(finalColor * DiffuseColor), 1.0f);
}