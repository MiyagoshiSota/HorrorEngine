#include "GeometryPass.h"

#include "Core/App.h"
#include "Modules/PublicConst/const_render_pref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Model/Model.h"

void GeometryPass::Collect(RenderContext& context)
{
	auto cmdList = context.CommandList;

	// パイプラインステートとルートシグネチャの設定
	auto name = "Geometry_Default";
	auto PSOname = "DefaultPipelinePass";

	// ルートシグネチャを設定
	cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(name)->get());

	// PSOを設定
	cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(PSOname)->Get());

	// 描画対象のオブジェクトを収集する
	m_RenderQueue.clear();
	for (auto& obj : context.GameObjects)
	{
		m_RenderQueue.push_back(obj);
	}

	// MSAA用のRenderTargetとDepthStencilを取得
	auto msaaColorRT = context.GetRenderTarget(const_render_pref::MSAART);
	auto msaaDepthRT = context.GetRenderTarget(const_render_pref::MSAA_Depth); // ★追加: MSAA用深度バッファ

	// RTVとDSVを書き込み可能状態に変更
	std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
	RendererUtility::simple_change_target_state(barriers, msaaColorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	RendererUtility::simple_change_target_state(barriers, msaaDepthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	if (!barriers->empty())
	{
		cmdList->ResourceBarrier(barriers->size(), barriers->data());
	}

	msaaColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
	msaaDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// Clear (MSAAバッファをクリアする)
	const float clearColor[] = { 0.0,0.0,0.0,1 };
	cmdList->ClearRenderTargetView(msaaColorRT->GetRTVHandle(), clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(msaaDepthRT->GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 出力先として MSAAターゲット を設定
	auto sceneDepthRHandle = msaaDepthRT->GetDSVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[] = { msaaColorRT->GetRTVHandle() };

	cmdList->OMSetRenderTargets(1, sceneColorRTVHandle, FALSE, &sceneDepthRHandle);
}

void GeometryPass::Draw(RenderContext& context)
{
	auto cmdList = context.CommandList;

	if (m_RenderQueue.empty()) {
		return;
	}

	UINT frameIndex = g_Engine->CurrentBackBufferIndex();

	// ライト情報の定数バッファをスロット1にセット
	const auto& lightingCB = g_Scene->get_lighting_manager()->get_constant_buffer();
	cmdList->SetGraphicsRootConstantBufferView(1, lightingCB->GetAddress());

	// view, proj行列の計算
	const auto view = DirectX::XMMatrixLookAtRH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(), context.Camera->GetUpward());
	const auto proj = DirectX::XMMatrixPerspectiveFovRH(context.Camera->GetFOV(), context.Camera->GetAspect(), 0.3f, 5000.0f);

	// マテリアルのディスクリプタヒープをセット
	auto materialHeap = g_Engine->GetDescriptorHeap()->GetHeap();
	cmdList->SetDescriptorHeaps(1, &materialHeap);

	for (auto& obj : m_RenderQueue)
	{
		// フレームインデックスを渡して、現在のフレーム用の定数バッファを取得します
		auto constantBuffer = obj->get_constant_buffer(frameIndex);

		// 取得したバッファを更新します
		auto pTransform = constantBuffer->GetPtr<SharedStruct::Transform>();
		pTransform->World = obj->get_transform();
		pTransform->View = view;
		pTransform->Proj = proj;
		pTransform->CameraPosition = context.Camera->GetEyePosFloat3();

		// 更新した定数バッファを GPU にセット
		cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetAddress());

		// オブジェクトを描画
		auto model = obj->find_component<MeshRenderer>()->model;
		auto origin_data = g_ModelLoader->GetModelOriginData(model->name);

		for (size_t i = 0; i < model->m_Meshes.size(); i++)
		{
			auto vbView = model->m_Meshes[i]->get_vertex_buffer()->View();
			auto ibView = model->m_Meshes[i]->get_index_buffer()->View();

			auto materialBuffer = model->m_Materials[i]->get_constant_buffer();
			auto pMaterial = materialBuffer->GetPtr<DirectX::XMFLOAT4>();
			pMaterial[0] = model->m_Materials[i]->get_color();
			cmdList->SetGraphicsRootConstantBufferView(2, materialBuffer->GetAddress());

			cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList->IASetVertexBuffers(0, 1, &vbView);
			cmdList->IASetIndexBuffer(&ibView);

			cmdList->SetGraphicsRootDescriptorTable(3, model->m_Materials[i]->get_srv_handle()->gpuHandle);

			cmdList->DrawIndexedInstanced(origin_data[i].Indeices.size(), 1, 0, 0, 0);
		}
	}

	auto msaaColorRT = context.GetRenderTarget(const_render_pref::MSAART);
	auto sceneColorRT = context.GetRenderTarget("SceneColor");

	if (sceneColorRT->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		D3D12_RESOURCE_BARRIER barrier[1] = { CD3DX12_RESOURCE_BARRIER::Transition(
			sceneColorRT->GetResource().Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		) };

		cmdList->ResourceBarrier(1, barrier);
	}

	// MSAA解決
	D3D12_RESOURCE_BARRIER resolveBarriers[2] = {};
	resolveBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		msaaColorRT->GetResource().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE
	);
	// SceneColor は SRV 状態から解決先に変更
	resolveBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		sceneColorRT->GetResource().Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RESOLVE_DEST
	);

	// バリアを設定
	cmdList->ResourceBarrier(2, resolveBarriers);

	// 解決処理
	cmdList->ResolveSubresource(
		sceneColorRT->GetResource().Get(), 0,
		msaaColorRT->GetResource().Get(), 0,
		sceneColorRT->GetResource()->GetDesc().Format
	);

	// 解決後の状態を元に戻す
	std::vector<D3D12_RESOURCE_BARRIER> barriersPost;
	barriersPost.push_back(
		CD3DX12_RESOURCE_BARRIER::Transition(
			msaaColorRT->GetResource().Get(),
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	);

	// SceneColor を SRV 状態に戻す
	barriersPost.push_back(
		CD3DX12_RESOURCE_BARRIER::Transition(
			sceneColorRT->GetResource().Get(),
			D3D12_RESOURCE_STATE_RESOLVE_DEST,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	);

	// バリアを設定
	cmdList->ResourceBarrier(barriersPost.size(), barriersPost.data());

	// 状態を更新
	sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
	msaaColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
}
