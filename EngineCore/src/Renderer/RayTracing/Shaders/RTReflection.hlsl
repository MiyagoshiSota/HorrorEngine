// RT Reflection: G-Buffer から反射方向を計算し TraceRay、ヒット時にアルベドを返す鏡面反射

RaytracingAccelerationStructure g_scene     : register(t0, space0);
Texture2D<float4> g_worldPosition           : register(t1, space0);
Texture2D<float4> g_normal                  : register(t2, space0);
Texture2D<float4> g_material                 : register(t3, space0);
RWTexture2D<float4> g_reflectionOutput      : register(u0, space0);

ByteAddressBuffer g_vertexBuffer : register(t0, space1);
ByteAddressBuffer g_indexBuffer  : register(t1, space1);
Texture2D<float4> g_albedoMap    : register(t2, space1);
SamplerState g_sampler          : register(s0, space0);

cbuffer RTReflectionConstants : register(b0, space0)
{
    float3 g_cameraPosition;
    float g_bias;
    float g_maxDistance;
    float g_reflectionIntensity;
    float g_roughnessThreshold;
    float g_fresnelF0;
    uint g_frameIndex;
    uint g_padding0_0;
    uint g_padding0_1;
    uint g_padding0_2;
    float3 g_skyColor;
    float g_padding1;
    uint g_vertexStrideBytes;
    uint g_padding2_0;
    uint g_padding2_1;
    uint g_padding2_2;
};

struct RTReflectionPayload
{
    float3 reflectionColor;
};

// Schlick のフレネル近似: F0 + (1 - F0) * (1 - NdotV)^5
float SchlickFresnel(float NdotV, float F0)
{
    const float power = 5.0;
    float f = pow(1.0 - saturate(NdotV), power);
    return saturate(lerp(F0, 1.0, f));
}

// SharedStruct::Vertex: Position(0), Normal(12), UV(24), Tangent(32), Color(44), stride=60
void LoadVertexAttributes(uint vertexIndex, out float3 outPosition, out float3 outNormal, out float2 outUV)
{
    // ストライドを計算（CBから渡される値を使用）
    const uint stride = max(1u, g_vertexStrideBytes);
    // オフセットを計算
    const uint o = vertexIndex * stride;

    // 位置を取得
    outPosition = asfloat(uint3(
        g_vertexBuffer.Load(o + 0),
        g_vertexBuffer.Load(o + 4),
        g_vertexBuffer.Load(o + 8)));
    // 法線を取得
    outNormal = asfloat(uint3(
        g_vertexBuffer.Load(o + 12),
        g_vertexBuffer.Load(o + 16),
        g_vertexBuffer.Load(o + 20)));
    // UVを取得
    outUV = asfloat(uint2(
        g_vertexBuffer.Load(o + 24),
        g_vertexBuffer.Load(o + 28)));
}

[shader("raygeneration")]
void RTReflectionRayGen()
{
    // レイを発射するインデックスを取得
    uint2 launchIndex = DispatchRaysIndex().xy;
    // ワールド座標を取得
    float4 worldPos4 = g_worldPosition[launchIndex];
    float3 worldPos = worldPos4.xyz;
    // 法線を取得
    float3 N = normalize(g_normal[launchIndex].xyz * 2.0 - 1.0);

    // 法線がほぼ0の場合は反射を無効化
    if (length(N) < 0.01)
    {
        // 反射を無効化
        g_reflectionOutput[launchIndex] = float4(1.0, 0.0, 0.0, 1.0);
        return;
    }

    // 粗さを取得
    float roughness = 1.0;
    
    // 粗さを取得
    if (g_material[launchIndex].x >= 0.0)
        roughness = g_material[launchIndex].x;
    
    // 粗さが閾値より大きい場合は反射を無効化
    if (roughness > g_roughnessThreshold)
    {
        g_reflectionOutput[launchIndex] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 反射方向を計算
    float3 V = normalize(worldPos - g_cameraPosition);
    float3 R = reflect(V, N);
    // バイアスを計算
    const float bias = max(0.001, g_bias);
    // 原点を計算
    float3 origin = worldPos + N * bias;

    // ペイロードを初期化
    RTReflectionPayload payload;
    payload.reflectionColor = float3(0.0, 0.0, 0.0);

    // レイを設定
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = normalize(R);
    ray.TMin = 0.0;
    ray.TMax = g_maxDistance;

    // レイをトレース（第5引数=1: GeometryIndex を SBT インデックスに反映しジオメトリ別ヒットグループを使用）
    TraceRay(g_scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 1, 0, ray, payload);

    // フレネル: 斜めから見るほど反射が強く、正面は弱く
    float NdotV = saturate(dot(N, V));
    float F = SchlickFresnel(NdotV, g_fresnelF0);

    // 反射カラーを出力（フレネルで強度を変調）
    g_reflectionOutput[launchIndex] = float4(payload.reflectionColor * g_reflectionIntensity * F, 1.0);
}

[shader("miss")]
void RTReflectionMiss(inout RTReflectionPayload payload)
{
    // スカイカラーを設定
    // payload.reflectionColor = g_skyColor;
    payload.reflectionColor = float3(0.0, 0.0, 0.0);
}

[shader("closesthit")]
void RTReflectionClosestHit(inout RTReflectionPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // プリミティブインデックスを取得
    const uint primIdx = PrimitiveIndex();
    const uint indexOffset = primIdx * 3u * 4u;
    const uint i0 = g_indexBuffer.Load(indexOffset);
    const uint i1 = g_indexBuffer.Load(indexOffset + 4u);
    const uint i2 = g_indexBuffer.Load(indexOffset + 8u);

    // 頂点属性を取得（位置もダミー変数で受ける）
    float3 p0, p1, p2;
    float3 n0, n1, n2;
    float2 uv0, uv1, uv2;
    LoadVertexAttributes(i0, p0, n0, uv0);
    LoadVertexAttributes(i1, p1, n1, uv1);
    LoadVertexAttributes(i2, p2, n2, uv2);

    // 重心座標の計算
    float w = 1.0 - attrib.barycentrics.x - attrib.barycentrics.y;
    float u = attrib.barycentrics.x;
    float v = attrib.barycentrics.y;

    // UV を補間
    float2 uv = w * uv0 + u * uv1 + v * uv2;

    // アルベドを取得して反射カラーにする
    float3 albedo = g_albedoMap.SampleLevel(g_sampler, uv, 0).rgb;
    payload.reflectionColor = saturate(albedo);
}
