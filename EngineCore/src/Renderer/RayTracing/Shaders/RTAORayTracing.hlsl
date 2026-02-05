// RTAO: G-Buffer（ワールド位置・法線）からレイトレースでアンビエントオクルージョンを計算
// 法線方向の半球上にレイを飛ばし、ヒットすれば遮蔽(0)、ミスなら非遮蔽(1)としてAO値を出力

RaytracingAccelerationStructure g_scene     : register(t0, space0);
Texture2D<float4> g_worldPosition           : register(t1, space0);
Texture2D<float4> g_normal                  : register(t2, space0);
RWTexture2D<float> g_aoOutput               : register(u0, space0);

cbuffer RTAOConstants : register(b0, space0)
{
    float3 g_cameraPosition;
    float g_radius;
    float g_bias;
    float g_padding0;
    float g_padding1;
    uint g_frameIndex;
    uint g_numRaysPerPixel;
};

struct RTAOPayload
{
    float occlusion; // 0=hit(occluded), 1=miss(unoccluded)
};

// ピクセル座標から簡易ハッシュで擬似乱数を生成（ジッター用）
float Hash(uint2 p, uint frame)
{
    uint n = p.x + p.y * 256u + frame * 65536u;
    n = (n ^ 61u) ^ (n >> 16u);
    n = n + (n << 3u);
    n = n ^ (n >> 4u);
    n = n * 0x27d4eb2du;
    n = n ^ (n >> 15u);
    return float(n) / 4294967296.0;
}

// 半球面上のコサイン重み付きランダム方向（法線基準）
float3 CosineWeightedHemisphere(float u1, float u2, float3 N)
{
    // ランダムオフセットを生成
    float phi = 2.0 * 3.14159265 * u1; // 方位角
    float cosTheta = sqrt(max(0.0, 1.0 - u2)); // 天頂角
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta)); // 半径
    
    // 法線を生成
    float3 H; // 法線
    H.x = cos(phi) * sinTheta; // 法線のX成分
    H.y = sin(phi) * sinTheta; // 法線のY成分
    H.z = cosTheta; // 法線のZ成分

    // 接空間からワールドへ（N を Z とする）
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0); // 上方向
    float3 T = normalize(cross(up, N)); // 接線
    float3 B = cross(N, T); // 副法線
    return normalize(T * H.x + B * H.y + N * H.z); // 法線を返す
}

// =============================================================================
// Ray Generation Shader
// =============================================================================
[shader("raygeneration")]
void RTAORayGen()
{
    // ピクセル座標を取得
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    // ワールド位置を取得
    float4 worldPos4 = g_worldPosition[launchIndex];
    float3 worldPos = worldPos4.xyz;
    
    // 法線を取得
    float4 normalEnc = g_normal[launchIndex];
    
    // 法線を[-1,1]の範囲に正規化
    float3 N = normalize(normalEnc.xyz * 2.0 - 1.0);

    // 背景または無効ピクセル（法線がほぼゼロ）
    if (length(N) < 0.01)
    {
        g_aoOutput[launchIndex] = 1.0;
        return;
    }

    float ao = 0.0;
    const uint numRays = max(1u, g_numRaysPerPixel); // レイの数(最低でも1つは飛ばす)

    // レイを飛ばす
    for (uint r = 0; r < numRays; ++r)
    {
        // ランダムオフセットを生成
        float u1 = Hash(launchIndex, g_frameIndex + r * 7u); // ランダムオフセット1(方位角用)
        float u2 = Hash(launchIndex + 1u, g_frameIndex + r * 13u); // ランダムオフセット2(天頂角用)

        // コサイン重み付き半球上の方向を生成
        float3 rayDir = CosineWeightedHemisphere(u1, u2, N);

        // レイの原点を生成
        float3 origin = worldPos + N * max(0.001, g_bias);

        // レイを定義
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = rayDir;
        ray.TMin = 0.0;
        ray.TMax = g_radius;

        // ペイロードを定義
        RTAOPayload payload;
        payload.occlusion = 1.0; // miss = unoccluded

        // レイを飛ばす
        TraceRay(
            g_scene,
            RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
            0xFF, 0, 0, 0,
            ray,
            payload
        );

        // 遮蔽量を加算
        ao += payload.occlusion;
    }

    // 遮蔽量を平均化
    ao /= float(numRays);
    // 遮蔽量を出力
    g_aoOutput[launchIndex] = ao;
}

// =============================================================================
// Miss Shader（何にも当たらなかった = 非遮蔽）
// =============================================================================
[shader("miss")]
void RTAOMiss(inout RTAOPayload payload)
{
    payload.occlusion = 1.0;
}

// =============================================================================
// Closest Hit Shader（当たった = 遮蔽）
// =============================================================================
[shader("closesthit")]
void RTAOClosestHit(inout RTAOPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    float dist = RayTCurrent();
    payload.occlusion = saturate(dist / g_radius); // 遮蔽量を計算
}
