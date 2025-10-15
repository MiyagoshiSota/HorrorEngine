#include "SceneSetupPass.h"

#include "Core/App.h"

void SceneSetupPass::Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context)
{
	auto name = "Geometry_Default";
    auto PSOname = "DefaultPipelinePass";

    // ルートシグネチャを設定
    cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(name)->get());

    // PSOを設定
    cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(PSOname)->Get());
}
