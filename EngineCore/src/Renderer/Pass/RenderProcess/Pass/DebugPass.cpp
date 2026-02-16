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
        const auto rb = obj->FindComponent<Rigidbody>();

        // Rigidbodyがあり、Colliderもある場合に追加
        if (rb && rb->GetColliderObject())
        {
            m_collidersToDraw.push_back(rb);
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

    // プリミティブトポロジを設定（線分リスト: 2頂点で1本の線）
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
}

void DebugPass::Draw(RenderContext& context)
{
    auto cmdList = context.CommandList;

    // 表示フラグ（COLLIDER_AABB）は DefaultScene::Update/EditorUpdate で update 前に設定済み
    const auto& lines = g_Scene->GetPhysicsWorld()->getDebugRenderer().getLines();
    if (lines.size() == 0)
    {
        return; // デバッグ線がなければ終了
    }

    UINT frameIndex = g_Engine->CurrentBackBufferIndex();

    m_currentFrameIndex = g_Engine->CurrentBackBufferIndex() % 2;
    m_currentObjectIndex = 0;

    // view, proj行列の計算 - 左手座標系に統一
    const auto view = DirectX::XMMatrixLookAtLH(context.Camera->GetEyePos(), context.Camera->GetTargetPos(),
                                                context.Camera->GetUpward());
    // TAA有効時はジッター適用済みの投影行列を使用
    const auto proj = context.GetProjectionMatrix();
    // HLSLは行ベクトル×行列で mul(pos, WorldViewProj) を使うため、転置して渡す（他パスと同様）
    const auto world = DirectX::XMMatrixTranspose(view * proj);

    auto constantBuffer = m_debugConstantBuffer->GetPtr<DebugConstants>();
    constantBuffer->world = world;
    cmdList->SetGraphicsRootConstantBufferView(0, m_debugConstantBuffer->GetAddress());

    {
        // メンバ変数の頂点バッファを使用
        m_debugTriangleVertices.clear();
        m_debugTriangleVertices.reserve(lines.size() * 2);

        UINT verticesToCopy = std::min(static_cast<UINT>(lines.size() * 2), kMaxDebugTriangleVertices);
        
        // 三角形 (3頂点) を正しく m_debugTriangleVertices に詰める
        for (UINT i = 0; i < lines.size() && (i*2) < verticesToCopy; ++i)
        {
            const auto& tri = lines[i];
            
            m_debugTriangleVertices.push_back({
                {tri.point1.x, tri.point1.y, tri.point1.z},
                {1, 0, 0, 1}
            });
            m_debugTriangleVertices.push_back({
                {tri.point2.x, tri.point2.y, tri.point2.z},
                {1, 0, 0, 1}
            });
        }

        // 頂点バッファのサイズ計算を修正
        auto stride = sizeof(DebugInfo);
        auto vertexBufferSize = stride * m_debugTriangleVertices.size();

        // 現在のフレーム用の頂点バッファを取得（スワップチェーンが3バッファでもオーバーランしないよう % で参照）
        const UINT vbIndex = frameIndex % kFrameBufferCount;
        auto currentVB = m_debugTriangleVertexBuffers[vbIndex];

        currentVB->CopyData(vertexBufferSize, m_debugTriangleVertices.data());
        
        // m_debugVertexBuffer ではなく currentVB を使う
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
