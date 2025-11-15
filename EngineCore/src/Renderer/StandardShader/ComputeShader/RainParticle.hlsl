// パーティクルの最大数 (C++側と一致させる)
static const uint MAX_PARTICLES = 100000;

// コンピュートシェーダーのスレッドグループのサイズ
// (C++側で Dispatch() を呼ぶ際の計算に使う)
static const uint THREAD_GROUP_SIZE = 256;

// パーティクルの構造体
struct Particle
{
    float3 position; // ワールド座標
    float3 velocity; // 速度ベクトル
    float life;      // 残り寿命 (0.0以下で消滅)
    float padding;   // (16バイトアライメントのためのパディング)
};

// パーティクルデータを格納するUAV (C++からバインド)
RWStructuredBuffer<Particle> g_Particles : register(u0);

// フレーム定数 (C++からバインド)
cbuffer FrameConstants : register(b0)
{
    float deltaTime;       // デルタタイム
    float3 windForce;      // 風の力 (例: float3(2.0, 0.0, 0.5))

    float3 emitCenter;     // 雨の発生領域の中心 (例: カメラ座標)
    float emitRadius;      // 発生領域の半径 (XZ平面)

    float emitHeight;      // 雨の発生するY座標
    float groundHeight;    // 地面のY座標
    float initialLifeMin;  // 寿命の最小値
    float initialLifeMax;  // 寿命の最大値
}

// 簡易的なハッシュ関数 (ランダム値生成用)
float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

// パーティクルの更新とリセットを行うコンピュートシェーダー
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void CS_UpdateParticles(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint particleIndex = dispatchThreadID.x;

    // バッファの範囲外アクセスを防ぐ
    if (particleIndex >= MAX_PARTICLES)
    {
        return;
    }

    // パーティクルデータを読み込む
    Particle p = g_Particles[particleIndex];

    // パーティクルが生存している場合の処理
    if (p.life > 0.0)
    {
        // 1-1. シミュレーション
        float3 gravity = float3(0.0, -9.8, 0.0);
        
        // 重力と風の影響を速度に加える
        p.velocity += (gravity + windForce) * deltaTime;
        
        // 速度を位置に反映
        p.position += p.velocity * deltaTime;
        
        // 寿命を減らす
        p.life -= deltaTime;

        // 1-2. 地面との衝突判定
        if (p.position.y < groundHeight)
        {
            // 衝突したら寿命を0にして消滅させる
            p.life = 0.0; 
            
            // 跳ね返らせる
            p.position.y = groundHeight;
            p.velocity.y = -p.velocity.y * 0.3; // 反射と減衰
        }
    }
    
    // --- 2. パーティクルが寿命切れの場合の処理 (リセット/再生成) ---
    if (p.life <= 0.0)
    {
        // 簡易的なランダムシードを生成
        float2 randomSeed = float2(particleIndex, deltaTime * 1000.0);
        
        // 新しい位置 (XZ平面のランダムな位置)
        float randomAngle = rand(randomSeed) * 2.0 * 3.14159265;
        float randomRadius = rand(randomSeed + 0.1) * emitRadius;
        
        p.position.x = emitCenter.x + cos(randomAngle) * randomRadius;
        p.position.z = emitCenter.z + sin(randomAngle) * randomRadius;
        p.position.y = emitHeight; // 高い位置からスタート

        // 2-2. 初速 (雨なので下向き)
        p.velocity = float3(0.0, -10.0, 0.0); 
        
        // 2-3. 新しい寿命 (ランダム化)
        p.life = lerp(initialLifeMin, initialLifeMax, rand(randomSeed + 0.2));
    }

    // 更新したデータをバッファに書き戻す
    g_Particles[particleIndex] = p;
}
