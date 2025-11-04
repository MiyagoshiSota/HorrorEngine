#include "RainParticleSystem.h"

#include "Core/App.h"
#include "Modules/Renderer/RendereUtility.h"

RainParticleSystem::RainParticleSystem()
{
    auto pDevice = g_Engine->Device();
    auto descriptorHeap = g_Engine->GetDescriptorHeap();

    // --- 1. パーティクルバッファ (DEFAULT ヒープ) の作成 ---
    const UINT particleBufferSize = MAX_PARTICLES * sizeof(Particle);

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = particleBufferSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // UAVとして使うため

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // GPU専用メモリ

    // m_pParticleBuffer の実体を作成
    // 初期状態は、アップロードバッファからのコピー先 (COPY_DEST)
    HRESULT hr = pDevice->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, // 初期状態
        nullptr,
        IID_PPV_ARGS(&m_pParticleBuffer));
    if (FAILED(hr))
    {
        printf("パーティクルバッファの作成に失敗\n");
        return;
    }
    m_pParticleBuffer->SetName(L"Particle Buffer");

    // アップロードバッファ (UPLOAD ヒープ) の作成
    D3D12_HEAP_PROPERTIES uploadHeapProps = {};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込めるメモリ

    D3D12_RESOURCE_DESC uploadBufferDesc = bufferDesc;
    uploadBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // UPLOAD ヒープは UAV にできない
    uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = pDevice->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, // CPUから書き込み、GPUから読み取り(コピー元)
        nullptr,
        IID_PPV_ARGS(&m_pUploadBuffer));
    if (FAILED(hr))
    {
        printf("Uploadバッファの作成に失敗\n");
        return;
    }
    m_pUploadBuffer->SetName(L"Particle Upload Buffer");

    // --- 2. 初期データの準備 (CPU側) ---
    // すべてのパーティクルの寿命を0にして、最初のフレームでリセットされるようにする
    std::vector<Particle> initialParticles(MAX_PARTICLES);
    for (auto& p : initialParticles)
    {
        p.life = 0.0f; 
    }
    
    // --- CPU から UPLOAD バッファへデータをコピー ---
    UINT8* pData;
    D3D12_RANGE readRange = { 0, 0 }; // CPUからは読み取らない
    hr = m_pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
    if (FAILED(hr))
    {
        printf("Map failed\n");
        return;
    }
    
    memcpy(pData, initialParticles.data(), particleBufferSize);
    m_pUploadBuffer->Unmap(0, nullptr);
    
    // CBVの確保
    constantBuffer = std::make_shared<ConstantBuffer>(sizeof(FrameConstants));
    sceneConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(SceneConstants));
}

void RainParticleSystem::Execute(RenderContext& context)
{
    auto cmdList = context.CommandList;
    const UINT particleBufferSize = MAX_PARTICLES * sizeof(Particle);
    // パイプラインステートとルートシグネチャの設定
    auto name = "Rain_Particles";
    auto PSOname = "RainParticlePass";

    // 定数バッファの更新
    auto cons = constantBuffer->GetPtr<FrameConstants>();
    const auto time_manager = g_Scene->get_time_manager();
    cons->deltaTime = time_manager->get_delta_time(); // 経過時間
    cons->windForce = DirectX::XMFLOAT3(0.5f, 0.0f, 0.0f); // 風の力
    cons->emitCenter = DirectX::XMFLOAT3(0.0f, 50.0f, 0.0f); // エミット中心位置
    cons->emitRadius = 300.0f; // エミット半径
    cons->emitHeight = 600.0f; // エミット高さ
    cons->groundHeight = 0.0f; // 地面の高さ
    cons->initialLifeMin = 5.0f; // パーティクルの初期寿命最小値
    cons->initialLifeMax = 10.0f; // パーティクルの初期寿命最大値

    // ルートシグネチャを設定
    cmdList->SetComputeRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(name)->get());

    // PSOを設定
    cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(PSOname)->Get());

    // 初回フレーム時のみ、UPLOAD バッファから DEFAULT バッファへデータをコピー
    if (firstFrame)
    {
        // UPLOAD バッファから DEFAULT バッファへコピー (GPUコマンド)
        cmdList->CopyBufferRegion(
            m_pParticleBuffer.Get(), // コピー先 (DEFAULT)
            0,
            m_pUploadBuffer.Get(),   // コピー元 (UPLOAD)
            0,
            particleBufferSize);

        // m_pParticleBuffer の状態を UAV (計算用) に遷移
        // コピー完了後、コンピュートシェーダーで使えるように状態遷移
        D3D12_RESOURCE_BARRIER particle_barrier = {};
        particle_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        particle_barrier.Transition.pResource = m_pParticleBuffer.Get();
        particle_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        particle_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        cmdList->ResourceBarrier(1, &particle_barrier);
        
        firstFrame = false;
    }
    
    // ルートパラメータの設定
    cmdList->SetComputeRootConstantBufferView(0, constantBuffer->GetAddress()); // b0
    cmdList->SetComputeRootUnorderedAccessView(1, m_pParticleBuffer->GetGPUVirtualAddress()); // u0

    // コンピュートシェーダーのディスパッチ
    UINT dispatchGroupsX = (MAX_PARTICLES + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;
    cmdList->Dispatch(dispatchGroupsX, 1, 1);
    
    Draw(context);
}

void RainParticleSystem::Draw(RenderContext& context)
{
    auto cmdList = context.CommandList;
    // パイプラインステートとルートシグネチャの設定
    auto name = "Rain_Particle_Render";
    auto PSOname = "RainParticleRenderPass";

    // RenderTargetの取得
    auto sceneColorRT = context.GetSourceRT();
    auto sceneDepthRT = context.GetDestRT();
    
    // シーン定数バッファの更新
    auto sceneConstant = sceneConstantBuffer->GetPtr<SceneConstants>();

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
    
    // 仮のカメラ情報
    sceneConstant->cameraPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    sceneConstant->projection = DirectX::XMMatrixPerspectiveFovRH(context.Camera->GetFOV(), context.Camera->GetAspect(), 0.3f,1000.0f);
    sceneConstant->view = DirectX::XMMatrixLookAtRH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(),context.Camera->GetUpward());
    sceneConstant->rainLength = 1.0f;

    // UAVからSRVへ状態遷移
    D3D12_RESOURCE_BARRIER srv_barrier = {};
    srv_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    srv_barrier.Transition.pResource = m_pParticleBuffer.Get();
    srv_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    srv_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(1, &srv_barrier);

    // 出力先としてレンダーターゲットと深度バッファを設定
    auto sceneDepthRHandle = sceneDepthRT->GetDSVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[] = { sceneColorRT->GetRTVHandle() };
    cmdList->OMSetRenderTargets(1, sceneColorRTVHandle, FALSE, &sceneDepthRHandle);

    // ルートシグネチャを設定
    cmdList->SetGraphicsRootSignature(g_Scene->get_pipeline_state_manager()->get_root_signature(name)->get());

    // PSOを設定
    cmdList->SetPipelineState(g_Scene->get_pipeline_state_manager()->get_pipeline_state(PSOname)->Get());

    // (ルートパラメータの設定)
    cmdList->SetGraphicsRootConstantBufferView(0, sceneConstantBuffer->GetAddress()); // b0
    cmdList->SetGraphicsRootShaderResourceView(1, m_pParticleBuffer->GetGPUVirtualAddress()); // t0

    // 描画コマンド
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
    cmdList->DrawInstanced(MAX_PARTICLES, 1, 0, 0);

    // SRVからCOPY_DESTへ状態遷移
    D3D12_RESOURCE_BARRIER particle_barrier = {};
    particle_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    particle_barrier.Transition.pResource = m_pParticleBuffer.Get();
    particle_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    particle_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    cmdList->ResourceBarrier(1, &particle_barrier);

    // 描画後、RTVとDSVを読み取り可能状態に変更
    std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriersOld = std::make_shared<std::vector<D3D12_RESOURCE_BARRIER>>();
    RendererUtility::simple_change_target_state(barriersOld, sceneColorRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(barriersOld->size(), barriersOld->data());
    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
