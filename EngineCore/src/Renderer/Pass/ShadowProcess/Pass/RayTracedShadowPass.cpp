#include "RayTracedShadowPass.h"
#include "Renderer/Engine.h"
#include "Scene/RayTracing/RayTracedShadowManager.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

namespace
{
    const float kShadowSceneWidth = 50.0f;
    const float kShadowSceneHeight = 50.0f;
    const float kShadowNearZ = 1.0f;
    // const float kShadowNearZ = 0.0f;
    const float kShadowFarZ = 150.0f;
    // const float kShadowFarZ = 10.0f;
    const float kShadowLightDistance = 25.0f;
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
            return;
        }
    }

    if (data.asManager->GetBottomLevelASList().size() == 0u)
    {
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
        return;
    }
    auto directionLight = lightManager->GetDirectionalLights()[0];
    DirectX::XMFLOAT3 lightDirF = directionLight->Direction;
    DirectX::XMVECTOR lightDir = DirectX::XMVector3Normalize(
        DirectX::XMVectorSet(lightDirF.x, lightDirF.y, lightDirF.z, 0.0f));
    DirectX::XMVECTOR targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR lightPos = DirectX::XMVectorSubtract(
        targetPos, DirectX::XMVectorScale(lightDir, kShadowLightDistance));
    // カメラ視点の逆 ViewProj（レイ方向用）
    const auto cameraView = DirectX::XMMatrixLookAtLH(
        context.Camera->GetEyePos(),
        context.Camera->GetTargetPos(),
        context.Camera->GetUpward());
    const auto cameraProj = context.GetProjectionMatrix();
    const DirectX::XMMATRIX cameraViewProj = DirectX::XMMatrixMultiply(cameraView, cameraProj);
    DirectX::XMVECTOR det;
    const DirectX::XMMATRIX invCameraViewProj = DirectX::XMMatrixInverse(&det, cameraViewProj);

    RayTracedShadowSceneConstants sceneConstants = {};
    DirectX::XMStoreFloat3(&sceneConstants.lightPosition, lightPos);
    sceneConstants.lightRadius = 1.0f;
    sceneConstants.lightDirection = lightDirF;
    DirectX::XMStoreFloat3(&sceneConstants.cameraPosition, context.Camera->GetEyePos());
    DirectX::XMStoreFloat4x4(&sceneConstants.invCameraViewProj, DirectX::XMMatrixTranspose(invCameraViewProj));
    const UINT frameIndex = g_Engine->CurrentBackBufferIndex();
    context.UpdateRayTracedShadowConstants(sceneConstants, frameIndex);

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
