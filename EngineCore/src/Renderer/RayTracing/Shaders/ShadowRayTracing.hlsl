// Ray Tracing Shader for Shadow Map Generation
// (ライト視点のシャドウマスク / デバッグ値を R32_FLOAT テクスチャに書き込む)

// -----------------------------------------------------------------------------
// Debug 出力モード
// -----------------------------------------------------------------------------
// SimplePS.hlsl 側の CalculateShadow() は、
//   g_useRayTracedShadow != 0 の場合に g_ShadowMap(R32_FLOAT) を
//   「0 = 影」「1 = 日向」のマスクとして参照している。
//
// そこで、本シェーダでは以下のモードで出力内容を切り替えられるようにする。
//
//   SHADOW_DEBUG_MODE = 0 : 通常モード（0=影, 1=日向 のマスク出力）
//   SHADOW_DEBUG_MODE = 1 : ヒット距離を可視化（0〜1 に正規化）
//   SHADOW_DEBUG_MODE = 2 : ヒット有無のみ出力（ヒット=1, ミス=0）
//   SHADOW_DEBUG_MODE = 3 : UV.x を出力（レイ生成の座標確認用）
//   それ以外           : 生の hitDist をそのまま出力（リニア距離）
//
// 必要に応じて、このファイル上部の定義を変更して再ビルドすれば
// すぐに別のデバッグモードを確認できる。
#ifndef SHADOW_DEBUG_MODE
#define SHADOW_DEBUG_MODE 1
#endif

// グローバルルートシグネチャ
RaytracingAccelerationStructure g_scene : register(t0, space0);
RWTexture2D<float> g_shadowOutput : register(u0); // float1 (R32_FLOAT 等を推奨)

cbuffer SceneConstants : register(b0)
{
    float3 g_lightPosition;
    float g_lightRadius;
    float3 g_lightDirection;
    float g_padding;
    row_major float4x4 g_invLightViewProj; // ライト視点の逆ViewProj
};

// ペイロード変更: ヒットフラグではなく「距離」を持つ
struct ShadowMapPayload
{
    float hitDist; // ライトから衝突点までの距離 (Linear Depth)
};

// =============================================================================
// Ray Generation Shader
// =============================================================================
[shader("raygeneration")]
void ShadowRayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    // 1. ライトの視錐台(Frustum)に基づいてレイの方向を計算
    // 現在の画素(UV)から、ライト視点のFar平面(z=1.0)上のワールド座標を求める
    float2 uv = (float2(launchIndex) + 0.5) / float2(launchDim);
    float4 ndc = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0);

    float4 targetWorldPos4 = mul(g_invLightViewProj, ndc);

    // w が 0 に極端に近い場合は逆変換が破綻している可能性があるので、
    // その画素だけ特別な値を書き込んで早期リターンする。
    // （R32_FLOAT なので、「-1」が出ているピクセルを見れば問題箇所を特定しやすい）
    if (abs(targetWorldPos4.w) < 1e-6f)
    {
        g_shadowOutput[launchIndex] = -1.0f;
        return;
    }

    float3 targetWorldPos = targetWorldPos4.xyz / targetWorldPos4.w;

    // ライト位置からターゲット方向へのベクトル
    float3 rayDir = normalize(targetWorldPos - g_lightPosition);

    // 2. レイの設定
    RayDesc ray;
    ray.Origin = g_lightPosition;
    ray.Direction = rayDir;
    ray.TMin = 0.01f;     // セルフシャドウノイズ回避用
    ray.TMax = 10000.0f;  // ライトの最大到達距離（シーンに合わせて十分大きく設定）

    // 3. ペイロード初期化（初期値は「無限遠」扱いにする）
    ShadowMapPayload payload;
    payload.hitDist = ray.TMax; 
    
    // 4. トレース実行
    // 「最も近い交点」を知りたいので、RAY_FLAG_CULL_BACK_FACING_TRIANGLES を使用。
    TraceRay(
        g_scene,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES, // 裏面は無視（必要に応じて RAY_FLAG_NONE）
        0xFF, 0, 0, 0,
        ray,
        payload
    );

    // 5. 出力
    // まず「ヒットがあったかどうか」を判定
    bool hasHit = (payload.hitDist < ray.TMax - 1e-3f);

#if SHADOW_DEBUG_MODE == 0
    // --- 通常モード ---
    // SimplePS.hlsl の CalculateShadow() が期待しているフォーマット:
    //   0.0 = 影, 1.0 = 日向
    g_shadowOutput[launchIndex] = hasHit ? 0.0f : 1.0f;

#elif SHADOW_DEBUG_MODE == 1
    // --- デバッグ: ヒット距離を 0〜1 に正規化して可視化 ---
    // シーンスケールに応じて maxDebugDist を調整する。
    const float maxDebugDist = 100.0f;
    float encoded = hasHit ? saturate(payload.hitDist / maxDebugDist) : 1.0f;
    g_shadowOutput[launchIndex] = encoded;

#elif SHADOW_DEBUG_MODE == 2
    // --- デバッグ: ヒット有無のみ ---
    // 1.0 = ヒットあり, 0.0 = ミス
    g_shadowOutput[launchIndex] = hasHit ? 1.0f : 0.0f;

#elif SHADOW_DEBUG_MODE == 3
    // --- デバッグ: レイ生成 UV.x の可視化 ---
    // 左端=0.0, 右端=1.0 が滑らかに変化するかで DispatchRays の範囲や
    // NDC/UV 変換の確認ができる。
    g_shadowOutput[launchIndex] = uv.x;

#else
    // --- フォールバック: 生の hitDist をそのまま出力 ---
    g_shadowOutput[launchIndex] = payload.hitDist;
#endif
}

// =============================================================================
// Miss Shader (何も当たらなかった場合)
// =============================================================================
[shader("miss")]
void ShadowMiss(inout ShadowMapPayload payload)
{
    // 何も当たらなかった場合は、RayGenで設定した初期値(TMax)のまま、
    // あるいは明示的に最大値を書き込む
    payload.hitDist = 10000.0f; 
}

// =============================================================================
// Any Hit Shader 
// =============================================================================
// 透明度テスト(Alpha Test)を行わない場合、Shadow Map作成においてAnyHitは不要です。
// 不透明ジオメトリのみなら削除したほうがパフォーマンスが良いです。
// もしパンチスルー(葉っぱなど)が必要ならここで IgnoreHit() などの処理をします。
[shader("anyhit")]
void ShadowAnyHit(inout ShadowMapPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // 何も当たらなかった場合は、RayGenで設定した初期値(TMax)のまま、
    // あるいは明示的に最大値を書き込む
    //payload.hitDist = 10000.0f; 
}

// =============================================================================
// Closest Hit Shader (最も近い交差点)
// =============================================================================
[shader("closesthit")]
void ShadowClosestHit(inout ShadowMapPayload payload, in BuiltInTriangleIntersectionAttributes attrib)
{
    // ライトから衝突点までの距離 (RayTCurrent) を記録
    payload.hitDist = RayTCurrent();
}