#include "GeometryPass.h"

void GeometryPass::Execute(ID3D12GraphicsCommandList* cmdList,RenderContext& context)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;

    // RenderTargetの取得
    auto sceneColorRT = context.GetRenderTarget("SceneColor");
    auto sceneDepthRT = context.GetRenderTarget("SceneDepth");

    if (sceneColorRT == nullptr || sceneDepthRT == nullptr)
    {
        printf("RenderTargetが見つかりませんでした。");
        return;
    }

    // 変更をcmdlistに追加
    // TODO:メソッド化したい。長い。
    if (sceneColorRT->GetCurrentState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
    {
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            sceneColorRT->GetResource().Get(),
            sceneColorRT->GetCurrentState(),
            D3D12_RESOURCE_STATE_RENDER_TARGET
        ));
    }
    if (sceneDepthRT->GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
    {
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            sceneDepthRT->GetResource().Get(),
            sceneDepthRT->GetCurrentState(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE
        ));
    }

    // 遷移が必要なバリアが1つ以上ある場合のみ実行
    if (!barriers.empty())
    {
        cmdList->ResourceBarrier(barriers.size(), barriers.data());
    }

	// 状態を更新
    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Clear
    const float clearColor[] = {0.5,0.5,0.5,1};
    cmdList->ClearRenderTargetView(sceneColorRT->GetRTVHandle(),clearColor,0,nullptr);
    cmdList->ClearDepthStencilView(sceneDepthRT->GetDSVHandle(),D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // 出力先としてレンダーターゲットと深度バッファを設定
    auto sceneDepthRHandle = sceneDepthRT->GetDSVHandle();
    //D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { sceneColorRT->GetRTVHandle() };
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[] = { g_Engine->GetCurrentRtvHandle() };
    cmdList->OMSetRenderTargets(1, rtvHandles, FALSE, &sceneDepthRHandle);
    
    const auto view = DirectX::XMMatrixLookAtRH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(), context.Camera->GetUpward());
    const auto proj = DirectX::XMMatrixPerspectiveFovRH(context.Camera->GetFOV(), context.Camera->GetAspect(), 0.3f, 1000.0f);

    UINT frameIndex = g_Engine->CurrentBackBufferIndex();

    if (!context.GameObjects.empty())
    {
        auto materialHeap = context.GameObjects[0]->GetModel()->m_Material->m_DescriptorHeap->GetHeap();
        cmdList->SetDescriptorHeaps(1, &materialHeap);
    }

    for (auto& obj : context.GameObjects)
    {
        // 2. フレームインデックスを渡して、現在のフレーム用の定数バッファを取得します
        auto constantBuffer = obj->GetConstantBuffer(frameIndex);

        // 取得したバッファを更新します
        auto pTransform = constantBuffer->GetPtr<SharedStruct::Transform>();
        pTransform->World = obj->GetTransform();
        pTransform->View = view;
        pTransform->Proj = proj;

        // 更新した定数バッファを GPU にセット
        cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer->GetAddress());

        // オブジェクトを描画
        context.Renderer->DrawGameObject(cmdList, obj);
    }

    // // RTVとDSVを読み取り可能状態に変更
    D3D12_RESOURCE_BARRIER barriersAfter[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            sceneColorRT->GetResource().Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,     // 現在の状態
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE // これからの状態
        ),
        CD3DX12_RESOURCE_BARRIER::Transition(
            sceneDepthRT->GetResource().Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_DEPTH_READ
        )
    };
    cmdList->ResourceBarrier(_countof(barriersAfter), barriersAfter);

    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_READ);
}
