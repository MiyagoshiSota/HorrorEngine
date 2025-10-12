#include "GeometryPass.h"

#include "Modules/Renderer/RendereUtility.h"

void GeometryPass::Execute(ID3D12GraphicsCommandList* cmdList,RenderContext& context)
{
    std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();

    // RenderTargetの取得
    auto sceneColorRT = context.GetRenderTarget("SceneColor");
    auto sceneDepthRT = context.GetRenderTarget("SceneDepth");
    if (sceneColorRT == nullptr || sceneDepthRT == nullptr)
    {
        printf("RenderTargetが見つかりませんでした。");
        return;
    }

    //  RTVとDSVを書き込み可能状態に変更
	RendererUtility::simple_change_target_state(barriers, sceneColorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	RendererUtility::simple_change_target_state(barriers, sceneDepthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // 遷移が必要なバリアが1つ以上ある場合のみ実行
     if (!barriers->empty())     
    {
        cmdList->ResourceBarrier(barriers->size(), barriers->data());
    }

     sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
	 sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Clear
    const float clearColor[] = {0.5,0.5,0.5,1};
    cmdList->ClearRenderTargetView(sceneColorRT->GetRTVHandle(),clearColor,0,nullptr);
    cmdList->ClearDepthStencilView(sceneDepthRT->GetDSVHandle(),D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 出力先としてレンダーターゲットと深度バッファを設定
    auto sceneDepthRHandle = sceneDepthRT->GetDSVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[] = { sceneColorRT->GetRTVHandle() };
    //D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[] = { g_Engine->GetCurrentRtvHandle() };
    cmdList->OMSetRenderTargets(1, sceneColorRTVHandle, FALSE, &sceneDepthRHandle);
    
    const auto view = DirectX::XMMatrixLookAtRH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(), context.Camera->GetUpward());
    const auto proj = DirectX::XMMatrixPerspectiveFovRH(context.Camera->GetFOV(), context.Camera->GetAspect(), 0.3f, 1000.0f);

    UINT frameIndex = g_Engine->CurrentBackBufferIndex();

    if (!context.GameObjects.empty())
    {
        //auto materialHeap = context.GameObjects[0]->get_model()->m_Material->m_DescriptorHeap->GetHeap();
		auto materialHeap = g_Engine->GetSrvHeap()->GetHeap();
        cmdList->SetDescriptorHeaps(1, &materialHeap);
    }

    for (auto& obj : context.GameObjects)
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

            cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            cmdList->IASetVertexBuffers(0, 1, &vbView);
            cmdList->IASetIndexBuffer(&ibView);

            cmdList->SetGraphicsRootDescriptorTable(1, model->m_Materials[i]->get_srv_handle()->gpuHandle);

            cmdList->DrawIndexedInstanced(model->m_InputMesh[i].Indeices.size(), 1, 0, 0, 0);
        }
    }

     // RTVとDSVを読み取り可能状態に変更
	//barriers->clear();
    std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriersOld = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();

	RendererUtility::simple_change_target_state(barriersOld, sceneColorRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	//RendererUtility::simple_change_target_state(barriersOld, sceneDepthRT, D3D12_RESOURCE_STATE_DEPTH_READ);
    cmdList->ResourceBarrier(barriersOld->size(), barriersOld->data());

    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    //sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_READ);
}
