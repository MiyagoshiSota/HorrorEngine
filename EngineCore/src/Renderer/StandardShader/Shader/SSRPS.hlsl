// SSR: SceneColor + Depth + G-Buffer からスクリーン空間で反射レイをマーチし、反射カラーを出力
// t0=SceneColor, t1=Depth, t2=Normal, t3=WorldPos, t4=Material
// b0=SSRConstants

Texture2D<float4> g_SceneColor : register(t0);
Texture2D<float> g_Depth : register(t1);
Texture2D g_Normal : register(t2);
Texture2D<float4> g_WorldPos : register(t3);
Texture2D g_Material : register(t4);

SamplerState g_Sampler : register(s0);

cbuffer SSRConstants : register(b0)
{
    float4x4 View;
    float4x4 InvView;
    float4x4 Projection;
    float4x4 InvProjection;
    float4 ProjectionParams; // x=farZ, y=1/farZ, z=screenWidth, w=screenHeight
    float NearZ;
    float FarZ;
    float MaxRayDistance;
    float RayStep;
    float MaxSteps;
    float Thickness;
    float Enable;
    float Padding[2];
};

struct PSInput
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// スクリーン UV と NDC 深度からビュー空間位置を復元
float3 ReconstructViewPos(float2 uv, float depthNDC)
{
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    float4 clipPos = float4(ndc.x, ndc.y, depthNDC, 1.0);
    float4 viewPosH = mul(InvProjection, clipPos);
    return viewPosH.xyz / viewPosH.w;
}

// ビュー空間位置をスクリーン UV と NDC 深度に投影
void ViewPosToScreen(float3 viewPos, out float2 screenUV, out float ndcDepth)
{
    float4 clipPos = mul(Projection, float4(viewPos, 1.0));
    float3 ndc = clipPos.xyz / clipPos.w;
    screenUV = float2(ndc.x * 0.5 + 0.5, -ndc.y * 0.5 + 0.5);
    ndcDepth = ndc.z;
}

float4 main(PSInput input) : SV_TARGET
{
    // 反射を無効化
    if (Enable < 0.5)
        return float4(0.0, 0.0, 0.0, 0.0);

    // 法線と粗さを取得
    float3 N = g_Normal.SampleLevel(g_Sampler, input.uv, 0).xyz * 2.0 - 1.0;
    float roughness = g_Material.SampleLevel(g_Sampler, input.uv, 0).r;

    // 粗さが0.85以上の場合は反射を無効化
    // if (roughness > 0.5)
    //     return float4(0.0, 0.0, 0.0, 0.0);

    // 深度を取得
    float depthNDC = g_Depth.SampleLevel(g_Sampler, input.uv, 0);
    float3 viewPos = ReconstructViewPos(input.uv, depthNDC);
    float3 viewNormal = normalize(mul((float3x3)View, N));
    float3 viewRayDir = normalize(viewPos);

    // 反射方向を計算
    float3 reflectDir = reflect(viewRayDir, viewNormal);
    // 反射方向が下向きの場合は反射を無効化
    if (reflectDir.z >= -0.01)
        return float4(0.0, 0.0, 0.0, 0.0);

    // ステップ長、最大ステップ数、厚み、最大距離を設定
    float stepLen = RayStep;
    int maxSteps = (int)MaxSteps;
    float thickness = Thickness;
    float maxDist = MaxRayDistance;

    // 反射レイをマーチング
    for (int i = 0; i < maxSteps; i++)
    {
        // ステップ距離を計算
        float dist = (float)(i + 1) * stepLen;
        if (dist > maxDist)
            break;

        // 反射レイの位置を計算
        float3 rayPos = viewPos + reflectDir * dist;

        // 反射レイの位置をスクリーン空間に投影
        float2 screenUV;
        float rayNDC;
        ViewPosToScreen(rayPos, screenUV, rayNDC);

        // 反射レイの位置がスクリーン外の場合は終了
        if (screenUV.x < 0.0 || screenUV.x > 1.0 || screenUV.y < 0.0 || screenUV.y > 1.0)
            break;

        // 反射レイの位置の深度を取得
        float sceneDepth = g_Depth.SampleLevel(g_Sampler, screenUV, 0);

        // 反射レイの位置の深度が0の場合は次のステップへ
        if (sceneDepth <= 0.0)
            continue;

        // 反射レイの位置の深度が厚みの範囲内の場合は反射カラーを取得
        if (rayNDC >= sceneDepth - thickness && rayNDC <= sceneDepth + thickness)
        {
            // 反射カラーを取得
            float3 reflectionColor = g_SceneColor.SampleLevel(g_Sampler, screenUV, 0).rgb;
            float fade = 1.0 - smoothstep(0.0, maxDist, dist);
            return float4(reflectionColor, fade * (1.0 - roughness));
        }

        // 反射レイの位置の深度が厚みの範囲外の場合は終了
        if (rayNDC > sceneDepth + thickness)
            break;
    }

    return float4(0.0, 0.0, 0.0, 0.0);
}
