#include "MonochromePass.h"


#include "Core/App.h"

#include "Renderer/Graphics/RootSignatureBuilder.h"


void MonochromePass::Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context)

{
	auto sceneColorRT = context.GetRenderTarget("SceneColor");
	if (sceneColorRT == nullptr)
	{
		printf("RenderTargetが見つかりませんでした。");
		return;
	}


	// 出力先としてバックバッファを指定
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { g_Engine->GetCurrentRtvHandle() };
	cmdList->OMSetRenderTargets(1, rtvHandles, FALSE, nullptr);

	auto rsignature_name = "PostProcess_SingleTexture";
	auto pso_name = "Monochrome";

	// パイプラインステートとルートシグネチャの設定
	cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(pso_name)->Get());
	cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(rsignature_name)->get());
	cmdList->SetGraphicsRootDescriptorTable(0, sceneColorRT->GetSRVHandle()->gpuHandle);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
}
