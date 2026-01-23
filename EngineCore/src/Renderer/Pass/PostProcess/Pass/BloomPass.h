#pragma once
#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"
#include <d3dx12.h>

#include "Core/App.h"

// パラメータ構造体
struct ExtractionPassParams
{
	float g_Threshold;
	float padding[3];
};

struct GaussianBlurPassParams
{
	DirectX::XMFLOAT2 g_Direction;
	float g_TextureWidth;
	float g_TextureHeight;
};

class Bloom : public PostProcessPassBase
{
public:
	Bloom() : PostProcessPassBase("Bloom", "PostProcess_TextureAndCBV")
	{
		// 定数バッファの初期化
		m_ExtractionPassCB = std::make_shared<ConstantBuffer>(sizeof(ExtractionPassParams));
		m_GaussianBlurPassHorizonCB = std::make_shared<ConstantBuffer>(sizeof(GaussianBlurPassParams));
		m_GaussianBlurPassVerticalCB = std::make_shared<ConstantBuffer>(sizeof(GaussianBlurPassParams));
	}

	void Execute(RenderContext& context) override
	{
		auto inputRT = context.GetSourceRT(); // 元画像
		auto destRT = context.GetDestRT();    // 最終出力先

		// 元画像のサイズ
		float width = static_cast<float>(inputRT->GetWidth());
		float height = static_cast<float>(inputRT->GetHeight());

		// フォーマット
		DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		// -------------------------------------------------------
		// 輝度抽出 (Extraction)
		// -------------------------------------------------------
		auto highLumRT = context.GetScopedTempRT(width / 2.0f, height / 2.0f, format);
		Extraction(context, inputRT, highLumRT.Get());

		// -------------------------------------------------------
		// ガウスぼかし (Gaussian Blur)
		// -------------------------------------------------------
		// Ping-Pong用のRTを2枚借りる (さらに1/4サイズにするとより効果的だが、今回は1/2で統一)
		auto blurTemp1 = context.GetScopedTempRT(width / 2.0f, height / 2.0f, format);
		auto blurTemp2 = context.GetScopedTempRT(width / 2.0f, height / 2.0f, format);

		// 横方向: HighLum -> Temp1
		GaussianBlur(context, highLumRT.Get(), blurTemp1.Get(), true);

		// 縦方向: Temp1 -> Temp2
		GaussianBlur(context, blurTemp1.Get(), blurTemp2.Get(), false);

		// -------------------------------------------------------
		// 3. 合成 (Combine)
		// -------------------------------------------------------
		Combine(context, inputRT, blurTemp2.Get(), destRT);
	}

	void LastExecute(RenderContext& context, D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle) override
	{
		auto inputRT = context.GetSourceRT(); // 元画像
		auto destRT = backBufferHandle;    // 最終出力先 (ToneMapping前のバッファなど)

		// 元画像のサイズ
		float width = static_cast<float>(inputRT->GetWidth());
		float height = static_cast<float>(inputRT->GetHeight());

		// フォーマット
		DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		// -------------------------------------------------------
		// 輝度抽出 (Extraction)
		// -------------------------------------------------------
		auto highLumRT = context.GetScopedTempRT(width / 2.0f, height / 2.0f, format);
		Extraction(context, inputRT, highLumRT.Get());

		// -------------------------------------------------------
		// ガウスぼかし (Gaussian Blur)
		// -------------------------------------------------------
		// Ping-Pong用のRTを2枚借りる (さらに1/4サイズにするとより効果的だが、今回は1/2で統一)
		auto blurTemp1 = context.GetScopedTempRT(width / 2.0f, height / 2.0f, format);
		auto blurTemp2 = context.GetScopedTempRT(width / 2.0f, height / 2.0f, format);

		// 横方向: HighLum -> Temp1
		GaussianBlur(context, highLumRT.Get(), blurTemp1.Get(), true);

		// 縦方向: Temp1 -> Temp2
		GaussianBlur(context, blurTemp1.Get(), blurTemp2.Get(), false);

		// -------------------------------------------------------
		// 3. 合成 (Combine)
		// -------------------------------------------------------
		CombineBackBuffer(context, inputRT, blurTemp2.Get(), destRT);
	}

	void ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT, const PostProcessParameter& params) override
	{
	}

private:

	// -------------------------------------------------------
	// 輝度抽出の実装
	// -------------------------------------------------------
	void Extraction(RenderContext& context, std::shared_ptr<ITargetBase> input, std::shared_ptr<RenderTarget> output)
	{
		auto cmdList = context.CommandList;

		// バリア遷移
		TransitionBarrier(cmdList, input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransitionBarrier(cmdList, output, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// RT設定 & ビューポート設定
		auto rtvHandle = output->GetRTVHandle();
		cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		SetViewport(cmdList, output);

		// PSO & RootSignature
		cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("BloomExtraction")->Get());
		cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("PostProcess_TextureAndCBV")->Get());

		// パラメータ
		ApplyExtractionParameters(GetCurrentParameters());
		cmdList->SetGraphicsRootConstantBufferView(0, m_ExtractionPassCB->GetAddress());

		// テクスチャ
		cmdList->SetGraphicsRootDescriptorTable(1, input->GetSRVHandle()->gpuHandle);

		DrawFullscreenQuad(cmdList);
	}

	// -------------------------------------------------------
	// ガウスぼかしの実装
	// -------------------------------------------------------
	void GaussianBlur(RenderContext& context, std::shared_ptr<RenderTarget> input, std::shared_ptr<RenderTarget> output, bool isHorizontal)
	{
		auto cmdList = context.CommandList;

		TransitionBarrier(cmdList, input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransitionBarrier(cmdList, output, D3D12_RESOURCE_STATE_RENDER_TARGET);

		auto rtvHandle = output->GetRTVHandle();
		cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		SetViewport(cmdList, output);

		cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("BloomBlur")->Get());
		cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("PostProcess_TextureAndCBV")->Get());

		ApplyGaussianBlurParameters(input, isHorizontal, GetCurrentParameters());

		if (isHorizontal)
		{
			cmdList->SetGraphicsRootConstantBufferView(0, m_GaussianBlurPassHorizonCB->GetAddress());
		}else
		{
			cmdList->SetGraphicsRootConstantBufferView(0, m_GaussianBlurPassVerticalCB->GetAddress());
		}
		
		cmdList->SetGraphicsRootDescriptorTable(1, input->GetSRVHandle()->gpuHandle);

		DrawFullscreenQuad(cmdList);
	}

	// -------------------------------------------------------
	// 合成の実装
	// -------------------------------------------------------
	void Combine(RenderContext& context, std::shared_ptr<ITargetBase> sceneColor, std::shared_ptr<RenderTarget> bloomColor, std::shared_ptr<ITargetBase> output)
	{
		auto cmdList = context.CommandList;

		TransitionBarrier(cmdList, sceneColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransitionBarrier(cmdList, bloomColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransitionBarrier(cmdList, output, D3D12_RESOURCE_STATE_RENDER_TARGET);

		auto rtvHandle = output->GetRTVHandle();
		cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		SetViewport(cmdList, output);

		// 合成用PSO (加算ブレンドなどを設定したPSO)
		cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("BloomCombine")->Get());
		// マルチテクスチャ用ルートシグネチャ (t0, t1を受け取る)
		cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("PostProcess_MultiTexture")->Get());

		// t0: 元画像
		cmdList->SetGraphicsRootDescriptorTable(0, sceneColor->GetSRVHandle()->gpuHandle);
		// t1: Bloom画像
		cmdList->SetGraphicsRootDescriptorTable(1, bloomColor->GetSRVHandle()->gpuHandle);

		DrawFullscreenQuad(cmdList);
	}

	void CombineBackBuffer(RenderContext& context, std::shared_ptr<ITargetBase> sceneColor, std::shared_ptr<RenderTarget> bloomColor, D3D12_CPU_DESCRIPTOR_HANDLE output)
	{
		auto cmdList = context.CommandList;

		TransitionBarrier(cmdList, sceneColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransitionBarrier(cmdList, bloomColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		cmdList->OMSetRenderTargets(1, &output, FALSE, nullptr);
		SetViewport(cmdList, output);

		// 合成用PSO (加算ブレンドなどを設定したPSO)
		cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("BloomCombine")->Get());
		// マルチテクスチャ用ルートシグネチャ (t0, t1を受け取る)
		cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("PostProcess_MultiTexture")->Get());

		// t0: 元画像
		cmdList->SetGraphicsRootDescriptorTable(0, sceneColor->GetSRVHandle()->gpuHandle);
		// t1: Bloom画像
		cmdList->SetGraphicsRootDescriptorTable(1, bloomColor->GetSRVHandle()->gpuHandle);

		DrawFullscreenQuad(cmdList);
	}

	// ヘルパー
	void DrawFullscreenQuad(ID3D12GraphicsCommandList* cmdList)
	{
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->DrawInstanced(3, 1, 0, 0);
	}

	// ビューポート設定ヘルパー
	void SetViewport(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> target)
	{
		D3D12_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(target->GetWidth());
		viewport.Height = static_cast<float>(target->GetHeight());
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissor = {};
		scissor.right = static_cast<LONG>(target->GetWidth());
		scissor.bottom = static_cast<LONG>(target->GetHeight());

		cmdList->RSSetViewports(1, &viewport);
		cmdList->RSSetScissorRects(1, &scissor);
	}

	// ウィンドウサイズ固定版
	void SetViewport(ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE targetHandle)
	{
		D3D12_VIEWPORT viewport = {};
		viewport.Width = static_cast<float>(kWindowWidth);
		viewport.Height = static_cast<float>(kWindowHeight);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D12_RECT scissor = {};
		scissor.right = static_cast<LONG>(kWindowWidth);
		scissor.bottom = static_cast<LONG>(kWindowHeight);
		
		cmdList->RSSetViewports(1, &viewport);
		cmdList->RSSetScissorRects(1, &scissor);
	}

	// バリアヘルパー (ITargetBase対応)
	void TransitionBarrier(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> target, D3D12_RESOURCE_STATES newState)
	{
		if (target->GetCurrentState() != newState)
		{
			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				target->GetResource().Get(),
				target->GetCurrentState(),
				newState
			);
			cmdList->ResourceBarrier(1, &barrier);
			target->SetCurrentState(newState);
		}
	}

	void ApplyExtractionParameters(const PostProcessParameter& params)
	{
		// ExtractionPassParamsの設定
		auto extractionShaderParams = m_ExtractionPassCB->GetPtr<ExtractionPassParams>();

		// "g_Threshold"という名前のパラメータを探して設定
		if (params.count("g_Threshold")) {
			extractionShaderParams->g_Threshold = params.at("g_Threshold");
		}
		else {
			extractionShaderParams->g_Threshold = 0.8f; // 見つからなければデフォルト値
		}
	};


	void ApplyGaussianBlurParameters(std::shared_ptr<ITargetBase> inputRT, bool isHorizontal, const PostProcessParameter& params)
	{
		GaussianBlurPassParams* blurParams = {};

		if (isHorizontal)
		{
			blurParams = m_GaussianBlurPassHorizonCB->GetPtr<GaussianBlurPassParams>();

			blurParams->g_Direction.y = 0.0f;
			if (params.count("x_param"))
			{
				blurParams->g_Direction.x = params.at("x_param");
			}
			else
			{
				blurParams->g_Direction.x = 1.0f; // デフォルト値
			}
		}
		else
		{
			// 縦方向
			blurParams = m_GaussianBlurPassVerticalCB->GetPtr<GaussianBlurPassParams>();

			blurParams->g_Direction.x = 0.0f;
			if (params.count("y_param"))
			{
				blurParams->g_Direction.y = params.at("y_param");
			}
			else
			{
				blurParams->g_Direction.y = 1.0f; // デフォルト値
			}
		}

		// テクスチャ幅と高さの設定
		blurParams->g_TextureWidth = static_cast<float>(inputRT->GetWidth());
		blurParams->g_TextureHeight = static_cast<float>(inputRT->GetHeight());
	}

private:
	std::shared_ptr<ConstantBuffer> m_ExtractionPassCB;
	std::shared_ptr<ConstantBuffer> m_GaussianBlurPassHorizonCB;
	std::shared_ptr<ConstantBuffer> m_GaussianBlurPassVerticalCB;
};