#pragma once
#include <d3d12.h>

#include "Core/App.h"
#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"

struct VHSPassParams {
    float g_ScanlineIntensity;
    float g_NoiseIntensity;
    float g_Time;
};

class VHSPass : public PostProcessPassBase
{
public:
	VHSPass() : PostProcessPassBase("VHS", "PostProcess_TextureAndCBV")
	{
		// Pass専用のConstantBufferを作成
		SetPassConstantBuffer(std::make_shared<ConstantBuffer>(sizeof(VHSPassParams)));
	}
	void ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT, const PostProcessParameter& params) override
	{
		// PostProcessManagerから渡された値で定数バッファを更新
		auto shaderParams = GetPassConstantBuffer()->GetPtr<VHSPassParams>();
		// "g_ScanlineIntensity"という名前のパラメータを探して設定
		if (params.count("g_ScanlineIntensity")) {
			shaderParams->g_ScanlineIntensity = params.at("g_ScanlineIntensity");
		}
		else {
			shaderParams->g_ScanlineIntensity = 0.5f; // 見つからなければデフォルト値
		}
		// "g_NoiseIntensity"という名前のパラメータを探して設定
		if (params.count("g_NoiseIntensity")) {
			shaderParams->g_NoiseIntensity = params.at("g_NoiseIntensity");
		}
		else {
			shaderParams->g_NoiseIntensity = 0.5f; // 見つからなければデフォルト値
		}
		// "g_Time"という名前のパラメータを探して設定
		if (params.count("g_Time")) {
			shaderParams->g_Time = params.at("g_Time");
		}
		else {
			shaderParams->g_Time = 0.0f; // 見つからなければデフォルト値
		}

		// ルートシグネチャに従ってリソースをセット
		// スロット0: このパス固有のパラメータ用定数バッファ (CBV)
		cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());
		// スロット1: 入力テクスチャ (SRV)
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
	}
};

