// Unjitter pixel shader
// TAA結果から現在フレームのジッターを除去し、「正しい位置」の画像を生成

cbuffer UnjitterParams : register(b0)
{
	float2 g_InvScreenSize;
	float2 g_Jitter; // ピクセル単位のジッター
};

Texture2D g_Input : register(t0);
SamplerState g_Sampler : register(s0);

struct PSInput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
	// 逆方向にサンプリングして「正しい位置」の色を取得
	float2 unjitterOffset = -g_Jitter * g_InvScreenSize;
	float2 sampleUV = input.TexCoord + unjitterOffset;
	
	// バイリニアフィルタリングでサンプリング
	return g_Input.Sample(g_Sampler, sampleUV);
}
