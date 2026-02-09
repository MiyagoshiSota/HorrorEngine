// RTGI: 1-bounce indirect + NEE (Next Event Estimation) for direct lights from G-Buffer.

RaytracingAccelerationStructure g_scene        : register(t0, space0);
Texture2D<float4> g_worldPosition             : register(t1, space0);
Texture2D<float4> g_normal                     : register(t2, space0);
Texture2D<float4> g_primaryAlbedo              : register(t3, space0);
RWTexture2D<float4> g_giOutput                 : register(u0, space0);

// Local Root (Hit Group): VB / IB / Albedo texture per geometry
ByteAddressBuffer g_vertexBuffer : register(t0, space1);
ByteAddressBuffer g_indexBuffer  : register(t1, space1);
Texture2D<float4> g_albedoMap     : register(t2, space1);
// グローバルルートの Static Sampler (s0, space0)
SamplerState g_sampler           : register(s0, space0);

cbuffer RTGIConstants : register(b0, space0)
{
    float3 g_cameraPosition;
    float g_radius;
    float g_bias;
    float g_indirectIntensity;
    float g_padding0;
    uint g_frameIndex;
    uint g_numRaysPerPixel;
    float3 g_skyColor;
    float g_padding1;
    uint g_vertexStrideBytes;
    uint g_padding2[3];
};

cbuffer LightParams : register(b1, space0)
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

struct RTGIPayload
{
    float3 indirectColor;
    uint occluded;
};

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

float3 CosineWeightedHemisphere(float u1, float u2, float3 N)
{
    float phi = 2.0 * 3.14159265 * u1;
    float cosTheta = sqrt(max(0.0, 1.0 - u2));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    return normalize(T * H.x + B * H.y + N * H.z);
}

[shader("raygeneration")]
void RTGIRayGen()
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

    // 法線がほぼゼロの場合は無視
    if (length(N) < 0.01)
    {
        g_giOutput[launchIndex] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    static const float PI = 3.14159265;
    float3 primaryAlbedo = g_primaryAlbedo[launchIndex].rgb;
    primaryAlbedo = saturate(primaryAlbedo);
    if (dot(primaryAlbedo, float3(1, 1, 1)) < 0.01)
        primaryAlbedo = float3(0.5, 0.5, 0.5);

    // --- NEE: 各光源へシャドウレイを飛ばし、非遮蔽なら直接光を加算 ---
    float3 accDirect = float3(0.0, 0.0, 0.0);
    const float bias = max(0.001, g_bias);
    const float3 origin = worldPos + N * bias;

    for (int i = 0; i < NumDirectionalLights && i < 4; ++i)
    {
        float3 L = normalize(-g_DirectionalLights[i].Direction.xyz);
        float NdotL = saturate(dot(N, L));
        if (NdotL < 0.0001) continue;

        RTGIPayload shadowPayload;
        shadowPayload.indirectColor = float3(0, 0, 0);
        shadowPayload.occluded = 0u;

        RayDesc shadowRay;
        shadowRay.Origin = origin;
        shadowRay.Direction = L;
        shadowRay.TMin = 0.0;
        shadowRay.TMax = 1e4;

        TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, shadowRay, shadowPayload);

        if (shadowPayload.occluded == 0u)
        {
            float3 radiance = g_DirectionalLights[i].ColorAndIntensity.rgb * g_DirectionalLights[i].ColorAndIntensity.a;
            accDirect += (primaryAlbedo / PI) * radiance * NdotL;
        }
    }

    for (int j = 0; j < NumPointLights && j < 28; ++j)
    {
        float3 lightPos = g_PointLights[j].Position.xyz;
        float3 toLight = lightPos - worldPos;
        float dist = length(toLight);
        float range = g_PointLights[j].AttenuationAndRange.y;
        if (dist >= range) continue;

        float3 L = toLight / max(dist, 1e-6);
        float NdotL = saturate(dot(N, L));
        if (NdotL < 0.0001) continue;

        RTGIPayload shadowPayload;
        shadowPayload.indirectColor = float3(0, 0, 0);
        shadowPayload.occluded = 0u;

        RayDesc shadowRay;
        shadowRay.Origin = origin;
        shadowRay.Direction = L;
        shadowRay.TMin = 0.0;
        shadowRay.TMax = max(0.0, dist - 0.001);

        TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, shadowRay, shadowPayload);

        if (shadowPayload.occluded == 0u)
        {
            float att = g_PointLights[j].AttenuationAndRange.x;
            float attenuation = 1.0 / (1.0 + att * dist * dist);
            float3 radiance = g_PointLights[j].ColorAndIntensity.rgb * g_PointLights[j].ColorAndIntensity.a * attenuation;
            accDirect += (primaryAlbedo / PI) * radiance * NdotL / max(dist * dist, 0.01);
        }
    }

    // --- 間接光: コサイン重み半球サンプリング ---
    float3 accIndirect = float3(0.0, 0.0, 0.0);
    const uint numRays = max(1u, g_numRaysPerPixel);

    for (uint r = 0; r < numRays; ++r)
    {
        float u1 = Hash(launchIndex, g_frameIndex + r * 7u);
        float u2 = Hash(launchIndex + 1u, g_frameIndex + r * 13u);
        float3 rayDir = CosineWeightedHemisphere(u1, u2, N);

        RTGIPayload payload;
        payload.indirectColor = float3(0.0, 0.0, 0.0);
        payload.occluded = 0u;

        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = rayDir;
        ray.TMin = 0.0;
        ray.TMax = g_radius;

        TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
        accIndirect += payload.indirectColor;
    }

    accIndirect /= float(numRays);
    accIndirect *= g_indirectIntensity;

    g_giOutput[launchIndex] = float4(accDirect + accIndirect, 1.0);
}

[shader("miss")]
void RTGIMiss(inout RTGIPayload payload)
{
    payload.indirectColor = g_skyColor;
}

static const float PI = 3.14159265;

// SharedStruct::Vertex: Position(0), Normal(12), UV(24), Tangent(32), Color(44), stride=60
void LoadVertexAttributes(uint vertexIndex, out float3 outNormal, out float2 outUV)
{
    const uint stride = max(1u, g_vertexStrideBytes);
    const uint o = vertexIndex * stride;
    outNormal = asfloat(uint3(g_vertexBuffer.Load(o + 12), g_vertexBuffer.Load(o + 16), g_vertexBuffer.Load(o + 20)));
    outUV = asfloat(uint2(g_vertexBuffer.Load(o + 24), g_vertexBuffer.Load(o + 28)));
}

[shader("closesthit")]
void RTGIClosestHit(inout RTGIPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // レイの交差距離を取得
    float t = RayTCurrent();
    // レイの方向を取得
    float3 rayDir = WorldRayDirection();
    
    // プリミティブインデックスを取得
    const uint primIdx = PrimitiveIndex();
    const uint indexOffset = primIdx * 3u * 4u;
    const uint i0 = g_indexBuffer.Load(indexOffset);
    const uint i1 = g_indexBuffer.Load(indexOffset + 4u);
    const uint i2 = g_indexBuffer.Load(indexOffset + 8u);

    // 頂点属性（法線・UV）を取得
    float3 n0, n1, n2;
    float2 uv0, uv1, uv2;
    LoadVertexAttributes(i0, n0, uv0);
    LoadVertexAttributes(i1, n1, uv1);
    LoadVertexAttributes(i2, n2, uv2);

    float w = 1.0 - attrib.barycentrics.x - attrib.barycentrics.y;
    float u = attrib.barycentrics.x;
    float v = attrib.barycentrics.y;

    float3 N_obj = normalize(w * n0 + u * n1 + v * n2);
    float2 uv = w * uv0 + u * uv1 + v * uv2;

    // テクスチャからアルベドをサンプリング（UV参照）
    float3 albedo = g_albedoMap.SampleLevel(g_sampler, uv, 0).rgb;
    albedo = saturate(albedo);

    // オブジェクトからワールド空間への変換行列を取得
    float3x4 objToWorld = ObjectToWorld3x4();
    // 法線をワールド空間に変換
    float3 N_world = normalize(mul(N_obj, (float3x3)objToWorld));
    // 入射角を計算
    float cosTheta = saturate(dot(N_world, -rayDir));
    float invDist2 = 1.0 / max(t * t, 0.01);

    payload.occluded = 1u;
    payload.indirectColor = (albedo / PI) * cosTheta * invDist2;
}
