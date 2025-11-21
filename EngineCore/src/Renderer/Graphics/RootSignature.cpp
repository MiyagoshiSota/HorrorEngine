#include "RootSignature.h"
#include "Renderer/Engine.h"
#include "RootSignatureBuilder.h"
#include <d3dx12.h>

bool RootSignature::create(ID3D12Device* device, const std::shared_ptr<RootSignatureBuilder>& builder)
{
	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = static_cast<UINT>(builder->m_RootParameters.size());
	desc.pParameters = builder->m_RootParameters.data();
	desc.NumStaticSamplers = static_cast<UINT>(builder->m_StaticSamplers.size());
	desc.pStaticSamplers = builder->m_StaticSamplers.data();
	desc.Flags = builder->m_Flags;

	ComPtr<ID3DBlob> pBlob;
	ComPtr<ID3DBlob> pErrorBlob;

	auto hr = D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,
		pBlob.GetAddressOf(),
		pErrorBlob.GetAddressOf()
	);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			// エラーメッセージを文字列として取得して表示
			const char* errStr = (const char*)pErrorBlob->GetBufferPointer();
			printf("RootSignature Serialization Error: %s\n", errStr);
			OutputDebugStringA(errStr); // Visual Studioの出力ウィンドウにも出す
		}
		// エラー処理
		return false;
	}

	hr = device->CreateRootSignature(
		0,
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		IID_PPV_ARGS(m_pRootSignature.GetAddressOf())
	);
	if (FAILED(hr))
	{
		// エラー処理
		return false;
	}

	m_IsValid = true;
	return true;
}

bool RootSignature::is_valid() const
{
	return m_IsValid;
}

ID3D12RootSignature* RootSignature::get() const
{
	return m_pRootSignature.Get();
}
