#include "SceneSetupPass.h"

void SceneSetupPass::Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context)
{
    // ルートシグネチャを設定
    cmdList->SetGraphicsRootSignature(context.Renderer->GetRootSignature());

    // PSOを設定
    cmdList->SetPipelineState(context.Renderer->GetPipelineState());
}
