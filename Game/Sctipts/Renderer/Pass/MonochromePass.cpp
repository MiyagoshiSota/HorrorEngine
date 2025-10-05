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
	cmdList->OMSetRenderTargets(0, rtvHandles, FALSE, nullptr);

	// RootSignatureとビルダーを作成
	auto name = "MonochromePass";
	auto builder = std::make_shared<RootSignatureBuilder>();
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	CD3DX12_DESCRIPTOR_RANGE tableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // SRVを1つ、t0から
	builder->add_descriptor_table(1, &tableRange).add_static_sampler(sampler);

	// 使用するPSOを作成
	g_Scene->get_pipeline_state_manager()->new_create(L"../x64/Debug/SimpleMonochromeVS.cso", L"../x64/Debug/SimpleMonochromePS.cso",false, false, builder, name);

	// パイプラインステートとルートシグネチャの設定
	cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(name)->Get());
	cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(name)->get());

	cmdList->SetGraphicsRootDescriptorTable(0, sceneColorRT->GetSRVHandle()->gpuHandle );

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
}
