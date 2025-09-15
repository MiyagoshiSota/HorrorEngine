#include "RootSignature.h"
#include "Engine.h"
#include <d3dx12.h>

RootSignature::RootSignature()
{
	auto flag = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // アプリケーションの入力アセンブラを使用する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS; // ドメインシェーダーのルートシグネチャへアクセスを拒否する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS; //  ハルシェーダーのルートシグネチャへんアクセスを拒否する
	flag |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS; // ジオメトリシェーダーのルートシグネチャへんアクセスを拒否する


	CD3DX12_ROOT_PARAMETER rootParam[2] = {}; // 定数バッファとテクスチャの2
	rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_DESCRIPTOR_RANGE tableRange[1] = {}; // ディスクリプタテーブル
	tableRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // シェーダーリソースビュー
	rootParam[1].InitAsDescriptorTable(std::size(tableRange), tableRange, D3D12_SHADER_VISIBILITY_ALL);

	// スタティックサンプラー設定
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	// ルートシグネチャの設定(設定したいルートパラメータとスタティックサンプラーを入れる)
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = std::size(rootParam); // ルートパラメータの個数を入れる
	desc.NumStaticSamplers = 1; // サンプラーの個数を入れる
	desc.pParameters = rootParam; // ルートパラメータのポインタを入れる
	desc.pStaticSamplers = &sampler; // サンプラーのポインタを入れる
	desc.Flags = flag; // フラグを設定

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	// シリアライズ
	auto hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf()
	);
	if (FAILED(hr))
	{
		printf("ルートシグネチャシリアライズ失敗");
		return;
	}

	hr = g_Engine->Device()->CreateRootSignature(
		0,
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.GetAddressOf())
	);
	if (FAILED(hr))
	{
		printf("ルートシグネチャの生成に失敗");
		return;
	}

	m_IsValid = true;
}

bool RootSignature::IsValid()
{
	return m_IsValid;
}

ID3D12RootSignature* RootSignature::Get()
{
	return m_pRootSignature.Get();
}
