struct VS_OUTPUT
{
	float3 worldPos : POSITION;
	float3 velocity : VELOCITY;
	float  life     : LIFE;
};

struct Particle
{
	float3 position;
	float3 velocity;
	float life;
	float padding;
};

StructuredBuffer<Particle> g_ParticleBuffer : register(t0);

VS_OUTPUT main( uint vid : SV_VertexID )
{
	Particle p = g_ParticleBuffer[vid];

	VS_OUTPUT output;

	// 寿命が残っているパーティクルのみ処理
	if (p.life > 0.0)
	{
		output.worldPos = p.position;
		output.velocity = p.velocity;
		output.life = p.life;
	}else
	{
		// 寿命切れのパーティクルは位置をゼロに
		output.worldPos = float3(0.0, 0.0, 0.0);
		output.velocity = float3(0.0, 0.0, 0.0);
		output.life = 0.0;
	}

	return output;
}