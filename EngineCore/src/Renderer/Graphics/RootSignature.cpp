#include "RootSignature.h"
#include "Renderer/Engine.h"
#include "RootSignatureBuilder.h"
#include <d3dx12.h>
#include "Modules/DxHelper.h"

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

	try
	{
		ThrowIfFailed(D3D12SerializeRootSignature(
			&desc,
			D3D_ROOT_SIGNATURE_VERSION_1_0,
			pBlob.GetAddressOf(),
			pErrorBlob.GetAddressOf()
		));
	}
	catch (const std::exception& e)
	{
		if (pErrorBlob)
		{
			// エラーメッセージを文字列として取得して表示
			const char* errStr = (const char*)pErrorBlob->GetBufferPointer();
			printf("RootSignature Serialization Error: %s\n", errStr);
			OutputDebugStringA(errStr); // Visual Studioの出力ウィンドウにも出す
		}
		printf("Exception: %s\n", e.what());
		return false;
	}

	try
	{
		ThrowIfFailed(device->CreateRootSignature(
			0,
			pBlob->GetBufferPointer(),
			pBlob->GetBufferSize(),
			IID_PPV_ARGS(m_pRootSignature.GetAddressOf())
		));
	}
	catch (const std::exception& e)
	{
		printf("RootSignature Creation Error: %s\n", e.what());
		return false;
	}

	m_IsValid = true;
	return true;
}

bool RootSignature::is_valid() const
{
	return m_IsValid;
}

ID3D12RootSignature* RootSignature::Get() const
{
	return m_pRootSignature.Get();
}
