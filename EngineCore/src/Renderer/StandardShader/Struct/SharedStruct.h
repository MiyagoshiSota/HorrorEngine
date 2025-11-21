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

	struct DirectionalLightForShader {
		DirectX::XMFLOAT4 Direction;
		DirectX::XMFLOAT4 ColorAndIntensity;
	};

	struct PointLightForShader {
		DirectX::XMFLOAT4 Position;
		DirectX::XMFLOAT4 ColorAndIntensity;
		DirectX::XMFLOAT4 AttenuationAndRange; // x: Attenuation, y: Range
	};

	struct SpotLightForShader
	{
		DirectX::XMFLOAT4 Position;
		DirectX::XMFLOAT4 Direction;
		DirectX::XMFLOAT4 ColorAndIntensity;
		DirectX::XMFLOAT4 SpotAnglesAttenuationAndRange; // x: InnerAngle, y: OuterAngle z: Attenuation, w: Range
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
		DirectX::XMFLOAT3 CameraPosition; // カメラの位置
		float Padding; // 16バイトアライメント調整用
	};
	
	struct alignas(256) LightingParams
	{
		DirectX::XMFLOAT4 AmbientColor; // 環境光
		int NumDirectionalLights; // 平行光源の数
		int NumPointLights; // 点光源の数
		int NumSpotLights; // スポットライトの数
		float Padding[1]; // 16バイトアライメント調整用
		DirectionalLightForShader DirectionalLights[4]; // 最大4つ
		PointLightForShader PointLights[28];      // 最大28個
		SpotLightForShader SpotLights[28];        // 最大28個
	};

	struct Mesh
	{
		std::vector<Vertex> Vertices; // 頂点データの配列
		std::vector<uint32_t> Indeices; // インデックスの配列

		// PBR
		std::wstring hAlbedoMap; // アルベドマップのファイルパス
		std::wstring hNormalMap; // 法線マップのファイルパス
		std::wstring hMetallicMap; // メタリックマップのファイルパス
		std::wstring hRoughnessMap; // ラフネスマップのファイルパス
		std::wstring hAOMap; // AOマップのファイルパス
		std::wstring hEmissiveMap; // エミッシブマップのファイルパス
		bool HasAlbedoMap = false; // アルベドマップを持っているか
		bool HasNormalMap = false; // 法線マップを持っているか
		bool HasMetallicMap = false; // メタリックマップを持っているか
		bool HasRoughnessMap = false; // ラフネスマップを持っているか
		bool HasAOMap = false; // AOマップを持っているか
		bool HasEmissiveMap = false; // エミッシブマップを持っているか

		DirectX::XMFLOAT4 albedoFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
	};
};