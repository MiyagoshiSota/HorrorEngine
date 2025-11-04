struct PS_INPUT
{
	float4 pos : SV_Position;
	float2 uv  : TEXCOORD0;
};

struct VS_OUTPUT
{
	float3 worldPos : POSITION;
	float3 velocity : VELOCITY;
	float  life     : LIFE;
};

struct GSOutput
{
	float4 pos : SV_POSITION;
};

cbuffer SceneConstants : register(b0)
{
	float4x4 matView;
	float4x4 matProjection;
	float3 cameraPos;
	float g_RainLength; // 雨の筋の長さ
};

PS_INPUT CreateVertex(float3 worldPos, float2 uv, float4x4 view, float4x4 proj)
{
	PS_INPUT output;
	output.pos = mul(proj, mul(view, float4(worldPos, 1.0)));
	output.uv = uv;
	return output;
}

[maxvertexcount(4)]
void main(point VS_OUTPUT input[1],inout TriangleStream<PS_INPUT> stream)
{
	// 寿命切れのパーティクルは描画しない
	if (input[0].life <= 0.0)
	{
		return;
	}

	float3 pos = input[0].worldPos;
	float3 velocity = normalize(input[0].velocity);

	// 雨の筋の長さと幅
	float length = g_RainLength;
	float width = 0.2; // TODO:一旦仮

	// ビルボードの計算
	float3 particleToCam = normalize(pos - cameraPos);
	float3 right = normalize(cross(velocity, particleToCam));
	float3 up = velocity * length;

	// 4つの頂点を計算
	float3 p0 = pos + right;		// 右上
	float3 p1 = pos - right;		// 左上
	float3 p2 = pos + right - up;	// 右下
	float3 p3 = pos - right - up;	// 左下

	// 4頂点を三角形ストリームに追加(2つのトライアングル)
	// 頂点0
	stream.Append(CreateVertex(p0,float2(1.0,0.0),matView,matProjection));
	// 頂点1
	stream.Append(CreateVertex(p1,float2(0.0,0.0),matView,matProjection));
	// 頂点2
	stream.Append(CreateVertex(p2,float2(1.0,1.0),matView,matProjection));
	// 頂点3
	stream.Append(CreateVertex(p3,float2(0.0,1.0),matView,matProjection));
}