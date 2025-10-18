#pragma once
#include <d3dx12.h>
#include <DirectXMath.h>
#include "Modules/ComPtr.h"

class SharedStruct
{
public:
	struct Vertex
	{
		DirectX::XMFLOAT3 Position; // 位置座標
		DirectX::XMFLOAT3 Normal; // 法線
		DirectX::XMFLOAT2 UV; // uv座標
		DirectX::XMFLOAT3 Tangent; // 接空間
		DirectX::XMFLOAT4 Color; // 頂点色
		static const D3D12_INPUT_LAYOUT_DESC InputLayout;

	private:
		static const int InputElementCount = 5;
		static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
	};

	struct LightData
	{
		DirectX::XMFLOAT4 Position; // w要素はライトの種類や有効/無効フラグに使う
		DirectX::XMFLOAT4 Color;    // w要素は強さ
		DirectX::XMFLOAT4 Attenuation; // x=Range, y=Attenuationなど
	};

	struct alignas(256) TimeData
	{
		float DeltaTime;
		float TotalTime;
	};

	struct alignas(256) Transform
	{
		DirectX::XMMATRIX World; // ワールド行列
		DirectX::XMMATRIX View; // ビュー行列
		DirectX::XMMATRIX Proj; // 東映行列
	};
	
	struct alignas(256) LightingParams
	{
		DirectX::XMFLOAT4 AmbientColor; // 環境光
		int NumLights; // シーンに存在するライトの数
		LightData Lights[30]; // MAX_LIGHTSはシェーダーとC++で合わせた定数 
	};

	struct Mesh
	{
		std::vector<Vertex> Vertices; // 頂点データの配列
		std::vector<uint32_t> Indeices; // インデックスの配列
		std::wstring DiffuseMap; // テクスチャのファイルパス
	};
};