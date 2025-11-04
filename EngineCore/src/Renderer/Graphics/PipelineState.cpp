#include "PipelineState.h"
#include "Renderer/Engine.h"
#include <d3dx12.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

PipelineState::PipelineState()
{
	// パイプラインステートの設定
	descGS.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // ラスタライザーはデフォルト
	descGS.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングはなしs
	descGS.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // ブレンドステートもデフォルト
	descGS.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // 深度ステンシルはデフォルトを使う
	descGS.SampleMask = UINT_MAX;
	descGS.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形を描画
	descGS.NumRenderTargets = 1; // 描画対象は1
	descGS.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	descGS.SampleDesc.Count = 1; // サンプラーは1
	descGS.SampleDesc.Quality = 0;
}

bool PipelineState::IsValid()
{
	return m_IsValid;
}

void PipelineState::SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout)
{
	descGS.InputLayout = layout;
}

void PipelineState::SetRootSignature(ID3D12RootSignature* rootSignature)
{
	descGS.pRootSignature = rootSignature;
	descCS.pRootSignature = rootSignature;
}

void PipelineState::SetVS(std::wstring filePath)
{
	// 頂点シェーダー読み込み
	auto hr = D3DReadFileToBlob(filePath.c_str(), m_pVsBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("頂点シェーダーの読み込みに失敗");
		return;
	}

	descGS.VS = CD3DX12_SHADER_BYTECODE(m_pVsBlob.Get());
}

void PipelineState::SetPS(std::wstring filePath)
{
	// ピクセルシェーダー読み込み
	auto hr = D3DReadFileToBlob(filePath.c_str(), m_pPSBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("ピクセルシェーダーの読み込みに失敗");
		return;
	}

	descGS.PS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.Get());
}

void PipelineState::SetGS(std::wstring filePath)
{
	// ジオメトリシェーダー読み込み
	auto hr = D3DReadFileToBlob(filePath.c_str(), m_pPSBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("ジオメトリシェーダーの読み込みに失敗");
		return;
	}

	// TODO:一旦
	descGS.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	descGS.InputLayout = { nullptr, 0 };
	
	descGS.GS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.Get());
}

void PipelineState::SetCS(std::wstring filePath)
{
	// コンピュートシェーダー読み込み
	auto hr = D3DReadFileToBlob(filePath.c_str(), m_pPSBlob.GetAddressOf());
	if (FAILED(hr))
	{
		printf("コンピュートシェーダーの読み込みに失敗");
		return;
	}

	descCS.CS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.Get());
}

void PipelineState::CreateGraphicsPSO()
{
	// パイプラインステートを生成
	auto hr = g_Engine->Device()->CreateGraphicsPipelineState(&descGS, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		printf("パイプラインステートの生成に失敗");
		return;
	}

	m_IsValid = true;
}

void PipelineState::CreateComputePSO()
{
	// パイプラインステートを生成
	auto hr = g_Engine->Device()->CreateComputePipelineState(&descCS, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		printf("コンピュートパイプラインステートの生成に失敗");
		return;
	}

	m_IsValid = true;
}

ID3D12PipelineState* PipelineState::Get()
{
	return m_pPipelineState.Get();
}

void PipelineState::SetRenderTargetFormat(DXGI_FORMAT format)
{
	descGS.RTVFormats[0] = format;
}

void PipelineState::SetDepthStencilFormat(DXGI_FORMAT format)
{
	descGS.DSVFormat = format;
}

void PipelineState::SetWireFrame(bool wireFrame)
{
	descGS.RasterizerState.FillMode = wireFrame ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
}
