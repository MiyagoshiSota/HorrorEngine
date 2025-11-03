#include "GeometryPass.h"

#include "Core/App.h"
#include "Modules/Renderer/RendereUtility.h"

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

	// RenderTargetの取得
    auto sceneColorRT = context.GetSourceRT();
    auto sceneDepthRT = context.GetDestRT();

	// RTVとDSVを書き込み可能状態に変更
	std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
    RendererUtility::simple_change_target_state(barriers, sceneColorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    RendererUtility::simple_change_target_state(barriers, sceneDepthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // 遷移が必要なバリアが1つ以上ある場合のみ実行
    if (!barriers->empty())
    {
        cmdList->ResourceBarrier(barriers->size(), barriers->data());
    }

	// 状態を更新
    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Clear
    const float clearColor[] = { 0.0,0.0,0.0,1 };
    cmdList->ClearRenderTargetView(sceneColorRT->GetRTVHandle(), clearColor, 0, nullptr);
    cmdList->ClearDepthStencilView(sceneDepthRT->GetDSVHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 出力先としてレンダーターゲットと深度バッファを設定
    auto sceneDepthRHandle = sceneDepthRT->GetDSVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[] = { sceneColorRT->GetRTVHandle() };
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
    auto materialHeap = g_Engine->GetSrvHeap()->GetHeap();
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

        // 更新した定数バッファを GPU にセット
        cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetAddress());

        // オブジェクトを描画
        auto model = obj->get_model();
        for (size_t i = 0; i < model->m_InputMesh.size(); i++)
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

            cmdList->DrawIndexedInstanced(model->m_InputMesh[i].Indeices.size(), 1, 0, 0, 0);
        }
    }

	// 描画後、RTVとDSVを読み取り可能状態に変更
    auto sceneColorRT = context.GetRenderTarget("SceneColor");
    std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriersOld = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
    RendererUtility::simple_change_target_state(barriersOld, sceneColorRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(barriersOld->size(), barriersOld->data());
    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
