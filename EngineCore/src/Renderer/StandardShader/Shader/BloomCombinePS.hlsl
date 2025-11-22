Texture2D g_SceneTexture : register(t0);
Texture2D g_BloomTexture : register(t1);
SamplerState g_Sampler : register(s0);

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 Reinhard(float3 x)
{
    return x / (x + 1.0f);
}

float3 Uncharted2Tonemap(float3 x)
{
    float A = 0.15; // Shoulder Strength
    float B = 0.50; // Linear Strength
    float C = 0.10; // Linear Angle
    float D = 0.20; // Toe Strength
    float E = 0.02; // Toe Numerator
    float F = 0.30; // Toe Denominator
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 Hable(float3 x)
{
    float exposure_bias = 2.0f;
    float3 curr = Uncharted2Tonemap(x * exposure_bias);
    
    // 白（11.2）がちゃんと1.0になるように正規化する
    float3 whiteScale = 1.0f / Uncharted2Tonemap(11.2f);
    return curr * whiteScale;
}

// シンプル化されたバージョン
float3 GTTonemap(float3 x)
{
    float P = 1.0; // 最大輝度
    float a = 1.0; // コントラスト
    float m = 0.22; // 線形部分の始まり
    float l = 0.4; // 線形部分の長さ
    float c = 1.33; // 黒の締まり
    float b = 0.0; // 黒の浮き

    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float3 w0 = 1.0 - smoothstep(0.0, m, x);
    float3 w2 = step(m + l0, x);
    float3 w1 = 1.0 - w0 - w2;

    float3 T = m * pow(x / m, c) + b;
    float3 S = P - (P - S1) * exp(CP * (x - S0));
    float3 L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    // 元の絵を取得
    float3 sceneColor = g_SceneTexture.Sample(g_Sampler, uv).rgb;
    
    // Bloom画像を取得
    float3 bloomColor = g_BloomTexture.Sample(g_Sampler, uv).rgb;
	
	// 加算合成
    float3 finalColor = sceneColor + bloomColor;
    
    // トーンマッピング
    finalColor = ACESFilm(finalColor);

    return float4(finalColor, 1.0);
}