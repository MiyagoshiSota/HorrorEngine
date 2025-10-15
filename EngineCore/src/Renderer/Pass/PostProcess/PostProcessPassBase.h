#pragma once
#include "Core/App.h"
#include "Renderer/Pass/IRenderPass.h"

class PostProcessPassBase : public IRenderPass
{
public:
    PostProcessPassBase(const std::string& psoName, const std::string& rootSignatureName) : m_PsoName(psoName),m_RootSignatureName(rootSignatureName){};

    void Execute(RenderContext& context) final override
    {
		auto cmdList = context.CommandList;

        // 出力先を設定
        D3D12_CPU_DESCRIPTOR_HANDLE outputRT[] = { context.GetDestRT()->GetRTVHandle() };
        cmdList->OMSetRenderTargets(1, outputRT, FALSE, nullptr);

        // PSOとルートシグネチャを設定
        cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(m_PsoName)->Get());
        cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(m_RootSignatureName)->get());

        // 派生クラスに固有のパラメータ設定を依頼
        ApplyParameters(cmdList, context, context.GetSourceRT());

        // 画面全体に三角形を描画 (共通処理)
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    };

    void LastExecute(RenderContext& context, D3D12_CPU_DESCRIPTOR_HANDLE backBufferHandle)
    {
        auto cmdList = context.CommandList;

        // 出力先を設定
        D3D12_CPU_DESCRIPTOR_HANDLE outputRT[] = { backBufferHandle };
        cmdList->OMSetRenderTargets(1, outputRT, FALSE, nullptr);

        // PSOとルートシグネチャを設定
        cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(m_PsoName)->Get());
        cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(m_RootSignatureName)->get());

        // 派生クラスに固有のパラメータ設定を依頼
        ApplyParameters(cmdList, context, context.GetSourceRT());

        // 画面全体に三角形を描画 (共通処理)
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->DrawInstanced(3, 1, 0, 0);
    }

protected:
    // 固有のパラメータをシェーダーにセットする
    virtual void ApplyParameters(ID3D12GraphicsCommandList* cmdList, RenderContext& context, std::shared_ptr<ITargetBase> inputRT) {
        // パラメータがなければ何もしない
    }

private:
    std::string m_PsoName;
    std::string m_RootSignatureName;
};
