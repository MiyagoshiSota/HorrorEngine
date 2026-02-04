// Ray Tracing Shader for Shadow Map Generation (Camera-View)
// カメラ視点のスクリーン空間で、各ピクセルに対応するレイのヒット点から
// ライトへの可視性（シャドウレイ）をトレースし、0=影 / 1=日向 を出力する。

// グローバルルートシグネチャ
RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float> g_shadowOutput : register(u0); // R32_FLOAT: 0=影, 1=日向

cbuffer SceneConstants : register(b0)
{
    float3 g_lightPosition;
    float g_lightRadius;
    float3 g_lightDirection;
    float g_padding;
    float3 g_cameraPosition;
    float g_cameraPadding;
    row_major float4x4 g_invCameraViewProj; // カメラ視点の逆 ViewProj（レイ方向用）
};

// ペイロード: スクリーン座標とシャドウ結果、再帰深度（カメラレイ=0, シャドウレイ=1）
struct ShadowMapPayload
{
    uint2 launchIndex;
    float shadowResult;  // 0=影, 1=日向
    uint rayDepth;       // 0=primary, 1=shadow ray
};

// =============================================================================
// Ray Generation Shader（カメラ視点）
// =============================================================================
[shader("raygeneration")]
void ShadowRayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    // カメラの視錐台に基づき、このピクセルを通るレイを生成
    float2 uv = (float2(launchIndex) + 0.5) / float2(launchDim);
    float4 ndc = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);
    float4 targetWorldPos4 = mul(g_invCameraViewProj, ndc);

    if (abs(targetWorldPos4.w) < 1e-6f)
    {
        g_shadowOutput[launchIndex] = 0.0f; // 不正なピクセルは日向扱い
        return;
    }

    float3 targetWorldPos = targetWorldPos4.xyz / targetWorldPos4.w;
    float3 rayDir = normalize(targetWorldPos - g_cameraPosition);

    RayDesc ray;
    ray.Origin = g_cameraPosition;
    ray.Direction = rayDir;
    ray.TMin = 0.001f;
    ray.TMax = 10000.0f;

    ShadowMapPayload payload;
    payload.launchIndex = launchIndex;
    payload.shadowResult = 1.0f;
    payload.rayDepth = 0u;

    TraceRay(
        g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        0xFF, 0, 0, 0,
        ray,
        payload
    );
}

// =============================================================================
// Miss Shader（空：日向）
// =============================================================================
[shader("miss")]
void ShadowMiss(inout ShadowMapPayload payload)
{
    // rayDepth == 0 (カメラレイ) が空に抜けた場合
    if (payload.rayDepth == 0u)
    {
        g_shadowOutput[payload.launchIndex] = 1.0f;
    }
    // rayDepth == 1 (シャドウレイ) が遮蔽物に当たらなかった場合
    else
    {
        // ここに来たということは、ライトへの道がクリアであるということ
        payload.shadowResult = 1.0f; // 日向
    }
}


// =============================================================================
// Any Hit Shader
// =============================================================================
[shader("anyhit")]
void ShadowAnyHit(inout ShadowMapPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
}

// =============================================================================
// Closest Hit Shader
// プライマリレイ: ヒット点からライトへシャドウレイを発射し、結果を書き込む。
// シャドウレイ: 何かに当たったら影（shadowResult = 0）。
// =============================================================================
[shader("closesthit")]
void ShadowClosestHit(inout ShadowMapPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // カメラレイのヒット処理（rayDepth == 0）
    if (payload.rayDepth == 0u)
    {
        float3 hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        

        float3 toLight = g_lightPosition - hitPoint;
        float distToLight = length(toLight);
        float3 shadowRayDir = toLight / max(distToLight, 1e-5f);

        float3 shadowOrigin = hitPoint + 0.001f; 

        RayDesc shadowRay;
        shadowRay.Origin = shadowOrigin; 
        shadowRay.Direction = shadowRayDir;
        shadowRay.TMin = 0.001f; 
        shadowRay.TMax = distToLight - 0.001f;

        // デフォルトを「影(0)」にしておく
        payload.rayDepth = 1u;
        payload.shadowResult = 0.0f;

        uint rayFlags = RAY_FLAG_FORCE_OPAQUE 
                      | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
                      | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
                      | RAY_FLAG_CULL_BACK_FACING_TRIANGLES;

        TraceRay(
            g_scene,
            rayFlags,
            0xFF, 0, 0, 0, 
            shadowRay,
            payload
        );

        g_shadowOutput[payload.launchIndex] = payload.shadowResult;
    }
}
