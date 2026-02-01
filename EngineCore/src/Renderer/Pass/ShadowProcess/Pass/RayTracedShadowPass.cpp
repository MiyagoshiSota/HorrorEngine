#include "RayTracedShadowPass.h"
#include "Renderer/Engine.h"
#include "Scene/RayTracing/RayTracedShadowManager.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <cstdio>

namespace
{
    const float kShadowSceneWidth = 50.0f;
    const float kShadowSceneHeight = 50.0f;
    const float kShadowNearZ = 1.0f;
    const float kShadowFarZ = 150.0f;
    const float kShadowLightDistance = 25.0f;

    // デバッグログ: 初回とその後は N フレームごとに出力（0 で毎フレーム）
    static constexpr UINT kDebugLogIntervalFrames = 120u;
    static UINT s_debugFrameCount = 0u;
}

void RayTracedShadowPass::Execute(RenderContext& context)
{
    if (!m_enabled)
    {
        return;
    }

    const auto& data = context.GetRayTracedShadowData();
    if (!data.isValid)
    {
        return;
    }

    auto device = g_Engine->Device();
    auto commandList = g_Engine->GetDxrCommandList();
    if (!commandList)
    {
        return;
    }

    Microsoft::WRL::ComPtr<ID3D12Device5> device5;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device5))))
    {
        return;
    }

    if (!data.asManager->IsBuilt())
    {
        if (!data.asManager->BuildAccelerationStructures(device5.Get(), commandList, context.GameObjects))
        {
            printf("[RayTracedShadowPass] エラー: Acceleration Structureの構築に失敗しました (GameObjects=%zu)\n",
                context.GameObjects.size());
            return;
        }
        printf("[RayTracedShadowPass] AS構築完了 BLAS数=%zu\n", data.asManager->GetBottomLevelASList().size());
    }

    const size_t blasCount = data.asManager->GetBottomLevelASList().size();
    if (blasCount == 0u)
    {
        printf("[RayTracedShadowPass] 警告: BLASが0です。DispatchRaysをスキップします。\n");
        return;
    }

    ID3D12DescriptorHeap* heaps[] = { data.descriptorHeap };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    auto tlas = data.asManager->GetTopLevelAS();
    if (!tlas)
    {
        return;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = data.descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = tlas->GetGpuAddress();
    device->CreateShaderResourceView(nullptr, &srvDesc, cpuHandle);

    cpuHandle.ptr += descriptorSize;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
    uavDesc.Texture2D.MipSlice = 0;
    device->CreateUnorderedAccessView(
        data.shadowOutputResource,
        nullptr,
        &uavDesc,
        cpuHandle
    );

    // ライト行列（SimpleShadowMapPass と同一）
    auto lightManager = g_Scene->GetLightingManager();
    if (!lightManager || lightManager->GetDirectionalLights().empty())
    {
        if (s_debugFrameCount == 0u)
            printf("[RayTracedShadow] DEBUG: LightingManager または DirectionalLights が空\n");
        return;
    }
    auto directionLight = lightManager->GetDirectionalLights()[0];
    DirectX::XMFLOAT3 lightDirF = directionLight->Direction;
    DirectX::XMVECTOR lightDir = DirectX::XMVector3Normalize(
        DirectX::XMVectorSet(lightDirF.x, lightDirF.y, lightDirF.z, 0.0f));
    DirectX::XMVECTOR targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR lightPos = DirectX::XMVectorSubtract(
        targetPos, DirectX::XMVectorScale(lightDir, kShadowLightDistance));
    DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtRH(
        lightPos, targetPos, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicRH(
        kShadowSceneWidth, kShadowSceneHeight, kShadowNearZ, kShadowFarZ);
    DirectX::XMMATRIX lightViewProj = DirectX::XMMatrixMultiply(lightView, lightProj);
    DirectX::XMMATRIX invLightViewProj;
    DirectX::XMVECTOR det;
    invLightViewProj = DirectX::XMMatrixInverse(&det, lightViewProj);

    RayTracedShadowSceneConstants sceneConstants = {};
    DirectX::XMStoreFloat3(&sceneConstants.lightPosition, lightPos);
    sceneConstants.lightRadius = 1.0f;
    sceneConstants.lightDirection = lightDirF;
    DirectX::XMStoreFloat4x4(&sceneConstants.invLightViewProj, DirectX::XMMatrixTranspose(invLightViewProj));
    context.UpdateRayTracedShadowConstants(sceneConstants);

    // デバッグログ（初回と kDebugLogIntervalFrames ごと）
    const bool shouldLog = (s_debugFrameCount == 0u) || (kDebugLogIntervalFrames > 0u && (s_debugFrameCount % kDebugLogIntervalFrames) == 0u);
    if (shouldLog)
    {
        float detF;
        DirectX::XMStoreFloat(&detF, det);
        DirectX::XMFLOAT3 lightPosF;
        DirectX::XMStoreFloat3(&lightPosF, lightPos);
        DirectX::XMFLOAT3 lightDirNormF;
        DirectX::XMStoreFloat3(&lightDirNormF, lightDir);
        float dirLen = sqrtf(lightDirF.x * lightDirF.x + lightDirF.y * lightDirF.y + lightDirF.z * lightDirF.z);

        // 画面中央(NDC 0,0,1)を逆行列でワールドに変換（HLSL と同一: inv*ndc → C++では (inv*ndc)^T = ndc*inv^T）
        DirectX::XMVECTOR ndcCenter = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
        DirectX::XMVECTOR worldCenter = DirectX::XMVector4Transform(ndcCenter, DirectX::XMMatrixTranspose(invLightViewProj));
        DirectX::XMFLOAT4 worldCenterF;
        DirectX::XMStoreFloat4(&worldCenterF, worldCenter);
        float w = worldCenterF.w;
        float cx = (fabsf(w) > 1e-6f) ? (worldCenterF.x / w) : worldCenterF.x;
        float cy = (fabsf(w) > 1e-6f) ? (worldCenterF.y / w) : worldCenterF.y;
        float cz = (fabsf(w) > 1e-6f) ? (worldCenterF.z / w) : worldCenterF.z;

        printf("[RayTracedShadow] DEBUG frame=%u ---\n", s_debugFrameCount);
        printf("  lightDirection (raw)   = (%.4f, %.4f, %.4f) len=%.4f %s\n",
            lightDirF.x, lightDirF.y, lightDirF.z, dirLen, (fabsf(dirLen - 1.0f) > 0.01f) ? "<- NOT NORMALIZED" : "");
        printf("  lightDirection (norm)  = (%.4f, %.4f, %.4f)\n", lightDirNormF.x, lightDirNormF.y, lightDirNormF.z);
        printf("  lightPosition          = (%.4f, %.4f, %.4f)\n", lightPosF.x, lightPosF.y, lightPosF.z);
        printf("  det(invLightViewProj)   = %.6e (0=singular)\n", detF);
        printf("  dispatch               = %u x %u\n", data.width, data.height);
        printf("  invLightViewProj row0  = (%.4f, %.4f, %.4f, %.4f)\n",
            sceneConstants.invLightViewProj._11, sceneConstants.invLightViewProj._12,
            sceneConstants.invLightViewProj._13, sceneConstants.invLightViewProj._14);
        printf("  worldPos(NDC center)   = (%.4f, %.4f, %.4f) w=%.4f\n", cx, cy, cz, w);

        // --- 確認候補1: SimpleShadowMapPass と同一パラメータ・行列 ---
        printf("  [確認1] SimpleShadowMapPass と同一か ---\n");
        printf("    ortho params (XMMatrixOrthographicRH) = width=%.1f height=%.1f near=%.1f far=%.1f\n",
            kShadowSceneWidth, kShadowSceneHeight, kShadowNearZ, kShadowFarZ);
        printf("    lightDistance = %.1f (SimpleShadowMapPass は 25.0f 固定)\n", kShadowLightDistance);
        DirectX::XMFLOAT4X4 viewF, projF;
        DirectX::XMStoreFloat4x4(&viewF, lightView);
        DirectX::XMStoreFloat4x4(&projF, lightProj);
        printf("    lightView  row0 = (%.4f, %.4f, %.4f, %.4f)\n",
            viewF._11, viewF._12, viewF._13, viewF._14);
        printf("    lightProj  row0 = (%.4f, %.4f, %.4f, %.4f)\n",
            projF._11, projF._12, projF._13, projF._14);

        // --- 確認候補2: HLSL の invLightViewProj 解釈 ---
        printf("  [確認2] CB=transpose(inv), HLSL column-major => M=inv, mul(M,ndc)=inv*ndc\n");

        // --- 確認候補3: 正射影 near/far を OrthographicRH にそのまま渡しているか ---
        printf("  [確認3] OrthographicRH に渡している値 = near=%.1f far=%.1f (上記 ortho params と一致)\n",
            kShadowNearZ, kShadowFarZ);
        printf("  BLAS数 = %zu (1 GameObject = 1 BLAS。各BLAS内にモデル全サブメッシュを含む)\n", blasCount);
    }
    s_debugFrameCount++;

    commandList->SetPipelineState1(data.pipelineState->GetStateObject());

    ID3D12RootSignature* globalRootSignature = data.pipelineState->GetGlobalRootSignature();
    // レイトレーシングはコンピュート扱いなので SetComputeRootSignature を使用する
    if (globalRootSignature)
        commandList->SetComputeRootSignature(globalRootSignature);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = data.descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    commandList->SetComputeRootDescriptorTable(0, gpuHandle); // t0: TLAS
    gpuHandle.ptr += descriptorSize;
    commandList->SetComputeRootDescriptorTable(1, gpuHandle); // u0: Output
    if (data.sceneConstantBuffer)
        commandList->SetComputeRootConstantBufferView(2, data.sceneConstantBuffer->GetGPUVirtualAddress()); // b0: Constants

    if (data.pShadowOutputState && *data.pShadowOutputState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            data.shadowOutputResource,
            *data.pShadowOutputState,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );
        commandList->ResourceBarrier(1, &barrier);
        *data.pShadowOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    // ClearUnorderedAccessViewFloat: GPU ハンドルはシェーダー可視ヒープ、CPU ハンドルは非シェーダー可視ヒープである必要がある。
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (data.clearUavCpuHandle.ptr != 0)
    {
        commandList->ClearUnorderedAccessViewFloat(gpuHandle, data.clearUavCpuHandle, data.shadowOutputResource, clearColor, 0, nullptr);
    }

    auto dispatchDesc = data.shaderBindingTable->GetDispatchRaysDesc(data.width, data.height);
    commandList->DispatchRays(&dispatchDesc);
    // デバッグ: 初回のみ DispatchRays 完了をログ
    if (s_debugFrameCount == 1u)
        printf("[RayTracedShadowPass] DispatchRays 完了 (出力→PIXEL_SHADER_RESOURCE)\n");

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        data.shadowOutputResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandList->ResourceBarrier(1, &barrier);
    if (data.pShadowOutputState)
    {
        *data.pShadowOutputState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}
