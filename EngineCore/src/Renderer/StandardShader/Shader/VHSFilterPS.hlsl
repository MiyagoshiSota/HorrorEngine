Texture2D g_SceneTexture : register(t0);
SamplerState g_SceneSampler : register(s0);

cbuffer TimeBuffer : register(b0)
{
    float g_Time;
};

float random(float2 uv)
{
    return frac(sin(dot(uv.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float noise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    float a = random(i);
    float b = random(i + float2(1.0, 0.0));
    float c = random(i + float2(0.0, 1.0));
    float d = random(i + float2(1.0, 1.0));
    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float4 main(float4 position : SV_Position,float2 texCoord : TEXCOORD) : SV_TARGET
{
    // 色収差
    float offset = 0.005;
    float r = g_SceneTexture.Sample(g_SceneSampler, float2(texCoord.x + offset, texCoord.y)).r;
    float g = g_SceneTexture.Sample(g_SceneSampler, texCoord).g;
    float b = g_SceneTexture.Sample(g_SceneSampler, float2(texCoord.x - offset, texCoord.y)).b;

    float3 finalColor = float3(r, g, b);

    // スキャンライン
	// 画面の奇数業を少し暗くして、走査線を表現
    float scanline = sin(texCoord.y * 800.0) * 0.1;
    finalColor -= scanline;

    // ビネット
    // 画面の恥に行くほど暗くして、ブラウン管の丸みを表現
    float2 centerDist = texCoord - 0.5;
    float vignette = 1.0 - dot(centerDist, centerDist) * 0.8;
    finalColor *= vignette;

    // ノイズ
	// 時間で変化するランダムなノイズを全体に加える
    float noiseVal = (noise(texCoord * g_Time) - 0.5) * 0.15;
    finalColor += noiseVal;

    // 全体を暗くしてガンマを調整
    finalColor = pow(saturate(finalColor), 1.5);

	return float4(finalColor, 1.0f);
}