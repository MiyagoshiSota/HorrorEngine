#pragma once
#include "Modules/ComPtr.h"
#include <d3dx12.h>
#include <string>

class PipelineState
{
public:
	PipelineState(); // コンストラクタである程度の設定をする
	bool IsValid(); // 生成に成功したかどうかを返す

	void SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout); // 入力レイアウトを設定
	void SetRootSignature(ID3D12RootSignature* rootSignature); // ルートシグネチャを設定
	void SetRenderTargetFormat(DXGI_FORMAT format);
	void SetDepthStencilFormat(DXGI_FORMAT format);
	void SetDepthFunc(D3D12_COMPARISON_FUNC func); // 深度比較関数を設定
	void SetWireFrame(bool wireFrame);
	void SetVS(std::wstring filePath); // 頂点シェーダーを設定
	void SetPS(std::wstring filePath); // ピクセルシェーダーを設定
	void SetGS(std::wstring filePath); // ジオメトリシェーダーを設定
	void SetCS(std::wstring filePath); // コンピュートシェーダーを設定
	void SetSampleDescCount(UINT count); // サンプル数を設定
	void SetFormat(DXGI_FORMAT format);
	void CreateGraphicsPSO(); // パイプラインステートを生成
	void CreateComputePSO(); // コンピュートパイプラインステートを生成
	void SetBlendEnable(bool blendEnable); // ブレンドを有効化するかどうか

	ID3D12PipelineState* Get();

private:
	bool m_IsValid = false; // 生成に成功したかどうか
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descGS = {}; // パイプラインステートの設定
	D3D12_COMPUTE_PIPELINE_STATE_DESC descCS = {}; // コンピュートパイプラインステートの設定
	ComPtr<ID3D12PipelineState> m_pPipelineState = nullptr; // パイプラインステート
	ComPtr<ID3DBlob> m_pVsBlob; // 頂点シェーダー
	ComPtr<ID3DBlob> m_pPSBlob; // ピクセルシェーダー
};