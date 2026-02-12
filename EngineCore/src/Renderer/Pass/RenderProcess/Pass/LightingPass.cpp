#include "LightingPass.h"
#include "Core/App.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Renderer/Engine.h"
#include "Scene/ISceneBase.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Renderer/Target/RenderTarget.h"
#include <d3dx12.h>

namespace
{
    // GeometryPass / SimpleShadowMapPass / DefaultPipelineManager / RayTracedShadowPass と
    // 完全に同一のライト空間パラメータを使用してライト行列を計算する。
    // これにより、SimplePS と LightingPS の posLight / projCoords が一致し、
    // どちらのパスでも同じシャドウマップを参照できるようにする。
    bool CalculateMainLightViewProj(DirectX::XMMATRIX& outLightViewProj)
    {
        using namespace DirectX;

        const float kShadowSceneWidth = 50.0f;
        const float kShadowSceneHeight = 50.0f;
        const float kShadowNearZ = 1.0f;
        const float kShadowFarZ = 150.0f;
        const float kShadowLightDistance = 25.0f;

        auto lightManager = g_Scene->GetLightingManager();
        if (!lightManager || lightManager->GetDirectionalLights().empty())
        {
            return false;
        }

        XMFLOAT3 lightDirF = lightManager->GetDirectionalLights()[0]->Direction;
        XMVECTOR lightDir = XMVector3Normalize(
            XMVectorSet(lightDirF.x, lightDirF.y, lightDirF.z, 0.0f));

        XMVECTOR targetPos = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR lightPos = XMVectorSubtract(
            targetPos, XMVectorScale(lightDir, kShadowLightDistance));

        XMMATRIX lightView = XMMatrixLookAtRH(
            lightPos, targetPos, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        XMMATRIX lightProj = XMMatrixOrthographicRH(
            kShadowSceneWidth, kShadowSceneHeight, kShadowNearZ, kShadowFarZ);

        // GeometryPass と同様に View * Proj の順で掛ける
        outLightViewProj = XMMatrixMultiply(lightView, lightProj);
        return true;
    }

    void TransitionToSRV(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> rt)
    {
        if (!rt || !rt->GetResource())
            return;
        if (rt->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            return;
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            rt->GetResource(),
            rt->GetCurrentState(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        cmdList->ResourceBarrier(1, &barrier);
        rt->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    void TransitionToRTV(ID3D12GraphicsCommandList* cmdList, std::shared_ptr<ITargetBase> rt)
    {
        if (!rt || !rt->GetResource())
            return;
        if (rt->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET)
            return;
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            rt->GetResource(),
            rt->GetCurrentState(),
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &barrier);
        rt->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
}

LightingPass::LightingPass()
{
    m_lightingTransformCB = std::make_shared<ConstantBuffer>(sizeof(LightingTransformCB));
}

void LightingPass::Execute(RenderContext& context)
{
    auto cmdList = context.CommandList;
    const ShadowContext& sc = context.GetShadowContext();

    auto albedoRT = context.GetRenderTarget(ConstRenderPref::GBufferAlbedo);
    auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
    auto worldPosRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);
    auto motionRT = context.GetRenderTarget(ConstRenderPref::MotionVectorBuffer);
    auto materialRT = context.GetRenderTarget(ConstRenderPref::GBufferMaterial);
    auto emissiveRT = context.GetRenderTarget(ConstRenderPref::GBufferEmissive);
    auto shadowRT = context.GetRenderTarget(ConstRenderPref::ShadowMap);
    auto ssaoRT = context.GetRenderTarget(ConstRenderPref::SSAOBuffer);
    auto rtgiRT = context.GetRenderTarget(ConstRenderPref::RTGIBuffer);
    auto rtReflectionRT = context.GetRenderTarget(ConstRenderPref::RTReflectionBuffer);
    auto sceneColorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);

    if (!albedoRT || !normalRT || !worldPosRT || !sceneColorRT)
        return;
    if (!albedoRT->GetSRVHandle() || !normalRT->GetSRVHandle() || !worldPosRT->GetSRVHandle())
        return;
    if (!sceneColorRT->GetRTVHandle().ptr)
        return;

    TransitionToSRV(cmdList, albedoRT);
    TransitionToSRV(cmdList, normalRT);
    TransitionToSRV(cmdList, worldPosRT);
    if (motionRT)
        TransitionToSRV(cmdList, motionRT);
    if (materialRT)
        TransitionToSRV(cmdList, materialRT);
    if (emissiveRT)
        TransitionToSRV(cmdList, emissiveRT);
    if (shadowRT)
        TransitionToSRV(cmdList, shadowRT);
    if (ssaoRT)
        TransitionToSRV(cmdList, ssaoRT);
    if (rtgiRT)
        TransitionToSRV(cmdList, rtgiRT);
    if (rtReflectionRT)
        TransitionToSRV(cmdList, rtReflectionRT);
    TransitionToRTV(cmdList, sceneColorRT);

    LightingTransformCB* cb = static_cast<LightingTransformCB*>(m_lightingTransformCB->GetPtr());
    cb->CameraPosition = context.Camera->GetEyePosFloat3();
    cb->Padding0 = 0.0f;

    // SimplePS / GeometryPass と完全に同一のライト行列を再計算して使用する。
    // （ShadowContext.mainLightViewProj とは別に、forward と deferred の posLight を確実に一致させる）
    DirectX::XMMATRIX mainLightViewProj;
    if (CalculateMainLightViewProj(mainLightViewProj))
    {
        DirectX::XMStoreFloat4x4(&cb->LightViewProj, DirectX::XMMatrixTranspose(mainLightViewProj));
    }
    else
    {
        // ライトが無い場合は恒等行列をセットしておく（影なし扱い）
        DirectX::XMStoreFloat4x4(&cb->LightViewProj, DirectX::XMMatrixIdentity());
    }

    cb->ShadowMode = static_cast<int>(sc.mode);
    cb->InvRayTracedShadowMapSize = context.GetInvRayTracedShadowMapSize();
    cb->RTGIEnabled = context.IsRTGIEnabled() ? 1 : 0;
    cb->RTReflectionEnabled = context.IsRTReflectionEnabled() ? 1 : 0;

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = sceneColorRT->GetRTVHandle();
    cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    cmdList->SetPipelineState(context.PipelineStateManager->GetPipelineState("LightingPass")->Get());
    cmdList->SetGraphicsRootSignature(context.PipelineStateManager->GetRootSignature("Lighting_Default")->Get());

    ID3D12DescriptorHeap* heaps[] = { g_Engine->GetDescriptorHeap()->GetHeap() };
    cmdList->SetDescriptorHeaps(1, heaps);

    cmdList->SetGraphicsRootConstantBufferView(0, m_lightingTransformCB->GetAddress());
    cmdList->SetGraphicsRootConstantBufferView(1, g_Scene->GetLightingManager()->GetConstantBuffer()->GetAddress());

    cmdList->SetGraphicsRootDescriptorTable(2, albedoRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(3, normalRT->GetSRVHandle()->gpuHandle);
    cmdList->SetGraphicsRootDescriptorTable(4, worldPosRT->GetSRVHandle()->gpuHandle);
    if (motionRT && motionRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(5, motionRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(5, albedoRT->GetSRVHandle()->gpuHandle);
    if (materialRT && materialRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(6, materialRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(6, albedoRT->GetSRVHandle()->gpuHandle);
    if (emissiveRT && emissiveRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(7, emissiveRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(7, albedoRT->GetSRVHandle()->gpuHandle);
    if (shadowRT && shadowRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(8, shadowRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(8, albedoRT->GetSRVHandle()->gpuHandle);
    if (ssaoRT && ssaoRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(9, ssaoRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(9, albedoRT->GetSRVHandle()->gpuHandle);
    if (rtgiRT && rtgiRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(10, rtgiRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(10, albedoRT->GetSRVHandle()->gpuHandle);
    if (rtReflectionRT && rtReflectionRT->GetSRVHandle())
        cmdList->SetGraphicsRootDescriptorTable(11, rtReflectionRT->GetSRVHandle()->gpuHandle);
    else
        cmdList->SetGraphicsRootDescriptorTable(11, albedoRT->GetSRVHandle()->gpuHandle);

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->DrawInstanced(3, 1, 0, 0);
}
