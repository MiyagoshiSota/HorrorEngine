#pragma once

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"

struct MonochromePassParams {
    float intensity; // モノクロ化の強さ (0.0:元の色, 1.0:完全なモノクロ)
};

class MonochromePass : public PostProcessPassBase
{
public:
    MonochromePass() : PostProcessPassBase("Monochrome", "PostProcess_TextureAndCBV")
    {
        // Pass専用のConstantBufferを作成
        SetPassConstantBuffer(std::make_shared<ConstantBuffer>(sizeof(MonochromePassParams)));
    }
    void MonochromePass::ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT, const PostProcessParameter& params) override
    {
        // PostProcessManagerから渡された値で定数バッファを更新
        auto shaderParams = GetPassConstantBuffer()->GetPtr<MonochromePassParams>();
    
        // "intensity"という名前のパラメータを探して設定
        if (params.count("intensity")) {
            shaderParams->intensity = params.at("intensity");
        } else {
            shaderParams->intensity = 1.0f; // 見つからなければデフォルト値
        }
    
        // ルートシグネチャに従ってリソースをセット
        // スロット0: このパス固有のパラメータ用定数バッファ (CBV)
        cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());

        // スロット1: 入力テクスチャ (SRV)
        cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
    }

};

