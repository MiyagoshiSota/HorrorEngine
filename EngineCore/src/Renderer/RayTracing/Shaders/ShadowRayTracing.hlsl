// Ray Tracing Shader for Hard Shadows
// このシェーダーはDXRを使用してハードシャドウを計算します

// デバッグ: 段階的に return してテスト（0=本番, 1～5=ステップで早期 return）
#define SHADOW_RAYGEN_DEBUG_STEP 3

// グローバルルートシグネチャパラメータ
RaytracingAccelerationStructure g_scene : register(t0, space0);  // TLAS
RWTexture2D<float> g_shadowOutput : register(u0);                 // シャドウマップ出力

// シーン定数バッファ（シャドウマップ方式：ライト視点の逆行列でワールド位置を復元）
cbuffer SceneConstants : register(b0)
{
    float3 g_lightPosition;    // ライト位置
    float g_lightRadius;       // ライト半径（予約）
    float3 g_lightDirection;   // ライト方向
    float g_padding;
    float4x4 g_invLightViewProj; // ライト視点の逆 View*Proj（シャドウマップ texel → ワールド位置）
};

// レイペイロード構造体（シャドウレイ用）
struct ShadowPayload
{
    bool isHit;  // レイがジオメトリにヒットしたか
};

// レイ属性（交差時の補間パラメータ）
struct RayAttributes
{
    float2 barycentrics;
};

// =============================================================================
// Ray Generation Shader（シャドウマップ方式：各 texel でライト→シーンへレイ）
// =============================================================================
[shader("raygeneration")]
void ShadowRayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    // シャドウマップ UV [0,1] → NDC (DirectX: x [-1,1], y [1,-1], z=1 = far)
    float2 uv = (float2(launchIndex) + 0.5) / float2(launchDim);
    float4 ndc = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);

    // HLSL は列優先: C++ で転置して渡すと GPU では inv として読まれる → inv * ndc にする
    float4 worldPos4 = mul(g_invLightViewProj, ndc);
    float3 worldPos = worldPos4.xyz / worldPos4.w;

    // レイ：ライト位置 → そのワールド位置へ
    float3 toPoint = worldPos - g_lightPosition;
    float dist = length(toPoint);
    if (dist < 1e-5)
    {
        g_shadowOutput[launchIndex] = 0.2f; // ライト直下は常に明るい
        return;
    }
    float3 rayDir = toPoint / dist;

    RayDesc ray;
    ray.Origin = g_lightPosition;
    ray.Direction = rayDir;
    ray.TMin = 0.01f;   // セルフシャドウ回避
    ray.TMax = max(dist - 0.01f, 0.0f);

    ShadowPayload payload;
    payload.isHit = false;

    TraceRay(
        g_scene,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        0xFF, 0, 0, 0,
        ray,
        payload
    );

    // ヒット=影(0), ミス=日向(1)
    float shadowValue = payload.isHit ? 0.0f : 1.0f;
    g_shadowOutput[launchIndex] = shadowValue;
}

// =============================================================================
// Miss Shader (レイが何にもヒットしなかった場合)
// =============================================================================
[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    // レイがジオメトリにヒットしなかった = 影ではない
    payload.isHit = false;
}

// =============================================================================
// Any Hit Shader (レイがジオメトリと交差した場合)
// =============================================================================
[shader("anyhit")]
void ShadowAnyHit(inout ShadowPayload payload, in RayAttributes attrib)
{
    // シャドウレイなので、最初の交差で影と判定
    payload.isHit = true;
}

// =============================================================================
// Closest Hit Shader (最も近い交差点)
// =============================================================================
// ※ RAY_FLAG_SKIP_CLOSEST_HIT_SHADERを使用しているため、このシェーダーは呼ばれない
// しかし、パイプラインには必要なのでダミーとして定義
[shader("closesthit")]
void ShadowClosestHit(inout ShadowPayload payload, in RayAttributes attrib)
{
    payload.isHit = true;
}
