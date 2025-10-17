#pragma once
#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/Target/ITargetBase.h"
#include "Scene/ISceneBase.h"

class PostProcessPassBase : public IRenderPass
{
public:
    PostProcessPassBase(const std::string& psoName, const std::string& rootSignatureName) : m_PsoName(psoName),m_RootSignatureName(rootSignatureName){};

    /// <summary>
	/// 通常の描画パスとして実行する場合の関数
    /// </summary>
    /// <param name="context"></param>
    void Execute(RenderContext& context) final override
    {
		auto cmdList = context.CommandList;

        // 出力先を設定
        D3D12_CPU_DESCRIPTOR_HANDLE outputRT[] = { context.GetDestRT()->GetRTVHandle() };
        cmdList->OMSetRenderTargets(1, outputRT, FALSE, nullptr);

        // PSOとルートシグネチャを設定
        cmdList->SetPipelineState(context.PipelineStateManager->get_pipeline_state(m_PsoName)->Get());
        cmdList->SetGraphicsRootSignature(context.PipelineStateManager->get_root_signature(m_RootSignatureName)->get());

        // 派生クラスに固有のパラメータ設定を依頼
        ApplyParameters(cmdList, context, context.GetSourceRT(),GetCurrentParameters());

        // 画面全体に三角形を描画 (共通処理)
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    };

    /// <summary>
	/// バックバッファに直接描画する場合の実行関数
    /// </summary>
    /// <param name="context"></param>
    /// <param name="backBufferHandle"></param>
    void LastExecute(RenderContext& context, D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle)
    {
        auto cmdList = context.CommandList;

        // 出力先を設定
        D3D12_CPU_DESCRIPTOR_HANDLE outputRT[] = { backBufferHandle };
        cmdList->OMSetRenderTargets(1, outputRT, FALSE, nullptr);

        // PSOとルートシグネチャを設定
        cmdList->SetPipelineState(context.PipelineStateManager->get_pipeline_state(m_PsoName)->Get());
        cmdList->SetGraphicsRootSignature(context.PipelineStateManager->get_root_signature(m_RootSignatureName)->get());

        // 派生クラスに固有のパラメータ設定を依頼
        ApplyParameters(cmdList, context, context.GetSourceRT(),GetCurrentParameters());

        // 画面全体に三角形を描画 (共通処理)
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }
	std::string GetPsoName() const { return m_PsoName; }
	std::string GetRootSignatureName() const { return m_RootSignatureName; }

	// 現在のパラメータをセットする
	void SetParameters(const PostProcessParameter settings) { m_CurrentSettings = settings; }
    PostProcessParameter GetCurrentParameters() const { return m_CurrentSettings; }

    // パス定数バッファを取得する
    void SetPassConstantBuffer(std::shared_ptr<ConstantBuffer> cb) { m_PassConstantBuffer = cb; }
    std::shared_ptr<ConstantBuffer> GetPassConstantBuffer() const { return m_PassConstantBuffer; }
    
protected:
    // 固有のパラメータをシェーダーにセットする
    virtual void ApplyParameters(
        ID3D12GraphicsCommandList* cmdList,
        RenderContext& context,
        std::shared_ptr<ITargetBase> inputRT,
        const PostProcessParameter& params // ★ 追加
    ) = 0;

private:
    std::string m_PsoName;
    std::string m_RootSignatureName;
    PostProcessParameter m_CurrentSettings;
    std::shared_ptr<ConstantBuffer> m_PassConstantBuffer;
};
