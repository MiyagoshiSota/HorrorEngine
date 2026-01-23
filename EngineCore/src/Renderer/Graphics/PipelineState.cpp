#include "PipelineState.h"
#include "Renderer/Engine.h"
#include <d3dx12.h>
#include <d3dcompiler.h>
#include "Modules/DxHelper.h"

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
	ThrowIfFailed(D3DReadFileToBlob(filePath.c_str(), m_pVsBlob.GetAddressOf()));
	descGS.VS = CD3DX12_SHADER_BYTECODE(m_pVsBlob.Get());
}

void PipelineState::SetPS(std::wstring filePath)
{
	if (filePath == L"")
	{
		return;
	}
	
	// ピクセルシェーダー読み込み
	ThrowIfFailed(D3DReadFileToBlob(filePath.c_str(), m_pPSBlob.GetAddressOf()));
	descGS.PS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.Get());
}

void PipelineState::SetGS(std::wstring filePath)
{
	// ジオメトリシェーダー読み込み
	ThrowIfFailed(D3DReadFileToBlob(filePath.c_str(), m_pPSBlob.GetAddressOf()));

	// TODO:一旦
	descGS.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	descGS.InputLayout = { nullptr, 0 };
	
	descGS.GS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.Get());
}

void PipelineState::SetCS(std::wstring filePath)
{
	// コンピュートシェーダー読み込み
	ThrowIfFailed(D3DReadFileToBlob(filePath.c_str(), m_pPSBlob.GetAddressOf()));
	descCS.CS = CD3DX12_SHADER_BYTECODE(m_pPSBlob.Get());
}

void PipelineState::SetSampleDescCount(UINT count)
{
	descGS.SampleDesc.Count = count;
}

void PipelineState::SetFormat(DXGI_FORMAT format)
{
	descGS.RTVFormats[0] = format;
}

void PipelineState::CreateGraphicsPSO()
{
	// パイプラインステートを生成
	ThrowIfFailed(g_Engine->Device()->CreateGraphicsPipelineState(&descGS, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())));
	m_IsValid = true;
}

void PipelineState::CreateComputePSO()
{
	// パイプラインステートを生成
	ThrowIfFailed(g_Engine->Device()->CreateComputePipelineState(&descCS, IID_PPV_ARGS(m_pPipelineState.ReleaseAndGetAddressOf())));
	m_IsValid = true;
}

void PipelineState::SetBlendEnable(bool blendEnable)
{
	if (blendEnable)
	{
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE; // 全てのレンダーターゲットで同じ設定を使う

		// RenderTarget[0]
		D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
		rtBlendDesc.BlendEnable = TRUE; // ブレンディングを有効にする
		rtBlendDesc.LogicOpEnable = FALSE;

		// 色の合成方法
		rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;       // (Rs, Gs, Bs) * As
		rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // (Rd, Gd, Bd) * (1 - As)
		rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;          // 上記2つを加算する

		// アルファ値自体の合成方法
		rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		blendDesc.RenderTarget[0] = rtBlendDesc;

		descGS.BlendState = blendDesc;
	}
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