#pragma once
#include <DirectXMath.h>

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"

struct GaussianBlurPassParams
{
    DirectX::XMFLOAT2 g_Direction; // 横なら(1, 0), 縦なら(0, 1)
    float g_TextureWidth; // テクスチャ幅 (UV計算用)
    float g_TextureHeight;
};

class GaussianBlurPass : public PostProcessPassBase
{
public:
    GaussianBlurPass() : PostProcessPassBase("GaussianBlur", "PostProcess_TextureAndCBV")
    {
        // Pass専用のConstantBufferを作成
        SetPassConstantBuffer(std::make_shared<ConstantBuffer>(sizeof(GaussianBlurPassParams)));
    }

    void ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT, const PostProcessParameter& params) override
    {
        // PostProcessManagerから渡された値で定数バッファを更新
        auto shaderParams = GetPassConstantBuffer()->GetPtr<GaussianBlurPassParams>();
        // "direction"という名前のパラメータを探して設定
        if (params.count("direction_x") && params.count("direction_y")) {
            shaderParams->g_Direction = DirectX::XMFLOAT2(params.at("direction_x"), params.at("direction_y"));
        }
        else {
            shaderParams->g_Direction = DirectX::XMFLOAT2(1.0f, 0.0f); // 見つからなければデフォルト値(横方向)
        }
        // テクスチャの幅と高さを設定

        shaderParams->g_TextureWidth = static_cast<float>(inputRT->GetWidth());
        shaderParams->g_TextureHeight = static_cast<float>(inputRT->GetHeight());

        // ルートシグネチャに従ってリソースをセット
        // スロット0: このパス固有のパラメータ用定数バッファ (CBV)
        cmdList->SetGraphicsRootConstantBufferView(0, GetPassConstantBuffer()->GetAddress());
        // スロット1: 入力テクスチャ (SRV)
        cmdList->SetGraphicsRootDescriptorTable(1, inputRT->GetSRVHandle()->gpuHandle);
    }
};
