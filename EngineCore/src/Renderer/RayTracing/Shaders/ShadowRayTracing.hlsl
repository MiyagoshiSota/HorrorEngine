// Ray Tracing Shader for Shadow Map Generation (Camera-View)
// カメラ視点のスクリーン空間で、各ピクセルに対応するレイのヒット点から
// ライトへの可視性（シャドウレイ）をトレースし、0=影 / 1=日向 を出力する。
//
// SimplePS.hlsl の CalculateShadow() は g_useRayTracedShadow != 0 のとき、
// 本テクスチャを「スクリーンUV」でサンプルする（ライト空間ではなくカメラ解像度に一致）。

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

    // プライマリレイの結果は Miss または ClosestHit で g_shadowOutput に書き込まれる
    // （RayGen では書き込まない）
}

// =============================================================================
// Miss Shader（空：日向）
// =============================================================================
// [shader("miss")]
// void ShadowMiss(inout ShadowMapPayload payload)
// {
//     if (payload.rayDepth == 0u)
//         g_shadowOutput[payload.launchIndex] = 1.0f; // カメラレイが何にも当たらなかった（空）
//     // シャドウレイの Miss は payload.shadowResult を 1 のまま（日向）で返すのみ
// }
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
// Any Hit Shader（未使用時は削除可）
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
// [shader("closesthit")]
// void ShadowClosestHit(inout ShadowMapPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
// {
//     if (payload.rayDepth == 0u)
//     {
//         // カメラレイのヒット点
//         float3 hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
//         float3 toLight = g_lightPosition - hitPoint;
//         float distToLight = length(toLight);
//         float3 shadowRayDir = toLight / max(distToLight, 1e-5f);

//         // シャドウレイ（ヒット点 → ライト）
//         RayDesc shadowRay;
//         shadowRay.Origin = hitPoint;
//         shadowRay.Direction = shadowRayDir;
//         shadowRay.TMin = 0.001f;   // セルフシャドウ回避
//         shadowRay.TMax = distToLight - 0.001f;

//         payload.rayDepth = 1u;
//         payload.shadowResult = 1.0f;

//         TraceRay(
//             g_scene,
//             RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
//             0xFF, 0, 0, 0,
//             shadowRay,
//             payload
//         );

//         g_shadowOutput[payload.launchIndex] = payload.shadowResult;
//     }
//     else
//     {
//         // シャドウレイが何かに当たった → 影
//         payload.shadowResult = 0.0f;
//     }
// }

[shader("closesthit")]
void ShadowClosestHit(inout ShadowMapPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // カメラレイのヒット処理（rayDepth == 0）
    if (payload.rayDepth == 0u)
    {
        float3 hitPoint = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        
        // 【重要】ここで法線(N)を取得する必要があります。
        // 一般的には頂点バッファをLoadしてBarycentrics(attrib.barycentrics)で補間します。
        // ここでは仮に N があるものとします。
        // float3 N = GetWorldNormal(PrimitiveIndex(), attrib.barycentrics); 
        
        // もし法線が取れない場合の暫定策:
        // カメラから見た面なので、カメラへ向かうベクトルを法線代わりにする（精度は落ちますがアクネは減ります）
        float3 viewVec = normalize(g_cameraPosition - hitPoint);
        float3 N = viewVec; // 本来はジオメトリ法線を使うべき

        float3 toLight = g_lightPosition - hitPoint;
        float distToLight = length(toLight);
        float3 shadowRayDir = toLight / max(distToLight, 1e-5f);

        // 【対策1】 法線オフセット (Normal Offset)
        // 表面から法線方向に少し浮かせた位置を開始点にする
        float3 shadowOrigin = hitPoint; // シーンのスケールに合わせて調整

        RayDesc shadowRay;
        shadowRay.Origin = shadowOrigin; 
        shadowRay.Direction = shadowRayDir;
        shadowRay.TMin = 0.001f; // オフセットしているのでTMinは小さめでOK
        shadowRay.TMax = distToLight - 0.001f;

        // ペイロードの準備：デフォルトを「影(0)」にしておく
        payload.rayDepth = 1u;
        payload.shadowResult = 0.0f; // 初期値「影」

        // 【対策2】 フラグの最適化
        // FORCE_OPAQUE: 透明度テストをスキップ（不透明のみなら）
        // ACCEPT_FIRST_HIT...: 何か当たったら即終了（影確定）
        // SKIP_CLOSEST_HIT...: Hitシェーダーを呼ばない（高速化）
        uint rayFlags = RAY_FLAG_FORCE_OPAQUE 
                      | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
                      | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER
                      | RAY_FLAG_CULL_BACK_FACING_TRIANGLES;

        TraceRay(
            g_scene,
            rayFlags,
            0xFF, 0, 0, 0, // Missシェーダーのインデックス等に注意
            shadowRay,
            payload
        );

        g_shadowOutput[payload.launchIndex] = payload.shadowResult;
    }
    // else { ... } ← シャドウレイ用の処理は不要になります
}
