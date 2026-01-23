#include "DebugPass.h"

#include "Core/App.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Physics/Component/Rigidbody.h"
#include "Scene/GameObject/GameObject.h"

void DebugPass::Collect(RenderContext& context)
{
    auto cmdList = context.CommandList;

    auto name = "Debug_Default";
    auto PSOname = "DebugPipelinePass";

    // ルートシグネチャを設定
    auto rootSig = g_Scene->GetPipelineStateManager()->GetRootSignature(name);
    if (!rootSig) return;
    cmdList->SetGraphicsRootSignature(rootSig->Get());

    // PSOを設定
    auto pso = g_Scene->GetPipelineStateManager()->GetPipelineState(PSOname);
    if (!pso) return;
    cmdList->SetPipelineState(pso->Get());

    // 描画対象のオブジェクト (Rigidbodyを持つGameObject) を収集する
    m_collidersToDraw.clear();
    for (auto& obj : context.GameObjects)
    {
        const auto rb = obj->find_component<Rigidbody>();

        // Rigidbodyがあり、Colliderもある場合に追加
        if (rb && rb->GetColliderObject())
        {
           // m_collidersToDraw.push_back(rb);
        }
    }

    // RenderTargetの取得
    auto sceneColorRT = context.GetSourceRT();
    auto sceneDepthRT = context.GetDestRT();

    // RTVとDSVを書き込み可能状態に変更
    std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers = std::make_shared<std::vector<
        D3D12_RESOURCE_BARRIER>>();
    RendererUtility::simple_change_target_state(barriers, sceneColorRT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // RendererUtility::simple_change_target_state(barriers, sceneDepthRT, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // 遷移が必要なバリアが1つ以上ある場合のみ実行
    if (!barriers->empty())
    {
        cmdList->ResourceBarrier(barriers->size(), barriers->data());
    }

    // 状態を更新
    sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    // sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Clear は GeometryPass で行っているので、DebugPass では不要

    // 出力先としてレンダーターゲットと深度バッファを設定
    auto sceneDepthRHandle = sceneDepthRT->GetDSVHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRTVHandle[] = {sceneColorRT->GetRTVHandle()};
    cmdList->OMSetRenderTargets(1, sceneColorRTVHandle, FALSE, &sceneDepthRHandle);

    // プリミティブトポロジを設定
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DebugPass::Draw(RenderContext& context)
{
    auto cmdList = context.CommandList;

    if (m_collidersToDraw.empty())
    {
        return; // 描画対象がなければ終了
    }

    UINT frameIndex = g_Engine->CurrentBackBufferIndex();

    m_currentFrameIndex = g_Engine->CurrentBackBufferIndex() % 2;
    m_currentObjectIndex = 0;

    // view, proj行列の計算
    const auto view = DirectX::XMMatrixLookAtRH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(),
                                                context.Camera->GetUpward());
    const auto proj = DirectX::XMMatrixPerspectiveFovRH(context.Camera->GetFOV(), context.Camera->GetAspect(), 0.3f,
                                                        1000.0f);
    const auto world = view * proj;

    // Debug
    for (const auto & obj : g_Scene->GetGameObjects())
    {
        auto rb = obj->find_component<Rigidbody>();
        if (rb && rb->GetRigidbody())
        {
         //   rb->GetRigidbody()->setIsDebugEnabled(true);
        }
    }

    auto constantBuffer = m_debugConstantBuffer->GetPtr<DebugConstants>();
    constantBuffer->world = world;
    cmdList->SetGraphicsRootConstantBufferView(0, m_debugConstantBuffer->GetAddress());
    
    auto& debugRenderer = g_Scene->GetPhysicsWorld()->getDebugRenderer();
    debugRenderer.setIsDebugItemDisplayed(reactphysics3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);

    const auto& lines = debugRenderer.getLines();
    if (lines.size() == 0)  
    {
        return;
    }{
        // ★★★ [FIX] メンバ変数の頂点バッファを使用
        m_debugTriangleVertices.clear();
        m_debugTriangleVertices.reserve(lines.size() * 2);

        if(lines.size() * 2 > kMaxDebugTriangleVertices)
        {
            printf("Warning: Exceeded max debug line vertices!\n");
            // 超えた分のラインは無視する
        }
        UINT verticesToCopy = std::min(static_cast<UINT>(lines.size() * 2), kMaxDebugTriangleVertices);
        
        // ★★★ [FIX] 三角形 (3頂点) を正しく m_debugTriangleVertices に詰める
        for (UINT i = 0; i < lines.size() && (i*2) < verticesToCopy; ++i)
        {
            const auto& tri = lines[i];
            
            m_debugTriangleVertices.push_back({
                {tri.point1.x, tri.point1.y, tri.point1.z},
                {1, 0, 0, 1} // 色 (例: 赤)
            });
            m_debugTriangleVertices.push_back({
                {tri.point2.x, tri.point2.y, tri.point2.z},
                {1, 0, 0, 1} // 色 (例: 赤)
            });
        }

        // ★★★ [FIX] 頂点バッファのサイズ計算を修正
        auto stride = sizeof(DebugInfo);
        auto vertexBufferSize = stride * m_debugTriangleVertices.size();

        // ★★★ [FIX] 毎フレーム make_shared するのをやめる
        // m_debugVertexBuffer = std::make_shared<VertexBuffer>(vertexBufferSize, stride, m_debugVertices.data());
        
        // ★★★ [ADD] 現在のフレーム用の頂点バッファを取得
        auto currentVB = m_debugTriangleVertexBuffers[frameIndex];

        // ★★★ [ASSUMPTION] 
        // VertexBuffer クラスに、データを Map/memcpy/Unmap するための
        // CopyData のような関数があることを前提とします。
        // (実装例: void VertexBuffer::CopyData(void* data, size_t size) { memcpy(m_mappedPtr, data, size); })
        currentVB->CopyData(vertexBufferSize, m_debugTriangleVertices.data());
        
        // ★★★ [FIX] m_debugVertexBuffer ではなく currentVB を使う
        const auto vbView = currentVB->View();
        
        cmdList->IASetVertexBuffers(0, 1, &vbView);
        cmdList->DrawInstanced(static_cast<UINT>(m_debugTriangleVertices.size()), 1, 0, 0);
    }
    
    // RTVの状態遷移
    auto sceneColorRT = context.GetRenderTarget("SceneColor");
    std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriersOld = std::make_shared<std::vector<
        D3D12_RESOURCE_BARRIER>>();
    RendererUtility::simple_change_target_state(barriersOld, sceneColorRT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if (!barriersOld->empty())
    {
        // バリアが必要な場合のみ実行
        cmdList->ResourceBarrier(barriersOld->size(), barriersOld->data());
        sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
}

// reactphysics3d::Transform を DirectX::XMMATRIX に変換 
DirectX::XMMATRIX DebugPass::ToDXMatrix(const reactphysics3d::Transform& transform)
{
    reactphysics3d::Vector3 pos = transform.getPosition();
    reactphysics3d::Quaternion rot = transform.getOrientation();
    DirectX::XMVECTOR dxPos = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
    DirectX::XMVECTOR dxRot = DirectX::XMVectorSet(rot.x, rot.y, rot.z, rot.w);
    DirectX::XMMATRIX scaleMat = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(dxRot);
    DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslationFromVector(dxPos);
    return scaleMat * rotMat * transMat;
}
