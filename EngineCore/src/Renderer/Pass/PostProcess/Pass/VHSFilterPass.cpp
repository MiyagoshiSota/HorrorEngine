#include "VHSFilterPass.h"

#include "Core/App.h"

//void VHSFilterPass::Execute(ID3D12GraphicsCommandList* cmdList, RenderContext& context)
//{
//	auto tmpColorRT = context.GetRenderTarget("TmpColor");
//	if (tmpColorRT == nullptr)
//	{
//		printf("RenderTargetが見つかりませんでした。");
//		return;
//	}
//
//
//	// 出力先としてバックバッファを指定
//	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { g_Engine->GetCurrentRtvHandle() };
//	cmdList->OMSetRenderTargets(1, rtvHandles, FALSE, nullptr);
//
//	auto rsignature_name = "PostProcess_TextureAndCBV";
//	auto pso_name = "VHSFilter";
//
//	// パイプラインステートとルートシグネチャの設定
//	cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(pso_name)->Get());
//	cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(rsignature_name)->get());
//
//	// SRVとCBVの設定
//	auto timeConstantBuffer = g_Scene->get_time_manager()->get_constant_buffer();
//	cmdList->SetGraphicsRootConstantBufferView(0, timeConstantBuffer->GetAddress());
//	cmdList->SetGraphicsRootDescriptorTable(1, tmpColorRT->GetSRVHandle()->gpuHandle);
//
//	// 全画面三角形を描画
//	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//	cmdList->DrawInstanced(3, 1, 0, 0);
//}
