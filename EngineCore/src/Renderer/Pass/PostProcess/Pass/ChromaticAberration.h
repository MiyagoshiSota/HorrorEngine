#pragma once
#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"

struct FilmGrainPassParams
{
	float offset;
};

class ChromaticAberration : public PostProcessPassBase
{
public:
	ChromaticAberration() : PostProcessPassBase("ChromaticAberration", "PostProcess_TextureAndCBV")
	{
		// Pass専用のConstantBufferを作成
		SetPassConstantBuffer(std::make_shared<ConstantBuffer>(sizeof(FilmGrainPassParams)));
	}
	void ChromaticAberration::ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT, const PostProcessParameter& params) override
	{
		// PostProcessManagerから渡された値で定数バッファを更新
		auto shaderParams = GetPassConstantBuffer()->GetPtr<FilmGrainPassParams>();
		// "offset"という名前のパラメータを探して設定
		if (params.count("offset")) {
			shaderParams->offset = params.at("offset");
		}
		else {
			shaderParams->offset = 0.5f; // 見つからなければデフォルト値
		}
		// ルートシグネチャに従ってリソースをセット
		// スロット0: このパス固有のパラメータ用定数バッファ (CBV)
		cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());
		// スロット1: 入力テクスチャ (SRV)
		cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
	}
};
