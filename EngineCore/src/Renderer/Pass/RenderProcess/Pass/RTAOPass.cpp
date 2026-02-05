#include "RTAOPass.h"
#include "Renderer/Engine.h"
#include "Scene/RayTracing/RayTracedAOManager.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

void RTAOPass::Execute(RenderContext& context)
{
    if (!m_enabled)
        return;

    const RayTracedAORenderData& data = context.GetRayTracedAOData();
    if (!data.isValid)
        return;

    auto device = g_Engine->Device();
    auto commandList = g_Engine->GetDxrCommandList();
    if (!commandList)
        return;

    Microsoft::WRL::ComPtr<ID3D12Device5> device5;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device5))))
        return;

    if (!data.asManager->IsBuilt())
    {
        if (!data.asManager->BuildAccelerationStructures(device5.Get(), commandList, context.GameObjects))
            return;
    }

    if (data.asManager->GetBottomLevelASList().empty())
        return;

    auto worldPosRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);
    auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
    if (!worldPosRT || !normalRT || !worldPosRT->GetSRVHandle() || !normalRT->GetSRVHandle())
        return;

    ID3D12DescriptorHeap* heaps[] = { data.descriptorHeap };
    commandList->SetDescriptorHeaps(1, heaps);

    D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrvDesc = {};
    tlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    tlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    tlasSrvDesc.RaytracingAccelerationStructure.Location = data.asManager->GetTopLevelAS()->GetGpuAddress();
    device->CreateShaderResourceView(nullptr, &tlasSrvDesc, data.tlasSrvCpuHandle);

    RayTracedAOConstants constants = {};
    DirectX::XMStoreFloat3(&constants.cameraPosition, context.Camera->GetEyePos());
    constants.radius = m_radius;
    constants.bias = m_bias;
    constants.frameIndex = g_Engine->CurrentBackBufferIndex();
    constants.numRaysPerPixel = m_numRaysPerPixel;
    context.UpdateRayTracedAOConstants(constants, constants.frameIndex);

    commandList->SetPipelineState1(data.pipelineState->GetStateObject());
    ID3D12RootSignature* globalRootSig = data.pipelineState->GetGlobalRootSignature();
    if (globalRootSig)
        commandList->SetComputeRootSignature(globalRootSig);

    commandList->SetComputeRootDescriptorTable(0, data.tlasSrvGpuHandle);
    commandList->SetComputeRootDescriptorTable(1, worldPosRT->GetSRVHandle()->gpuHandle);
    commandList->SetComputeRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
    commandList->SetComputeRootDescriptorTable(3, data.aoUavGpuHandle);
    if (data.constantBuffer)
        commandList->SetComputeRootConstantBufferView(4, data.constantBuffer->GetGPUVirtualAddress());

    if (data.pAoOutputState && *data.pAoOutputState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            data.aoOutputResource,
            *data.pAoOutputState,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );
        commandList->ResourceBarrier(1, &barrier);
        *data.pAoOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    const float clearColor[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
    if (data.clearUavCpuHandle.ptr != 0)
        commandList->ClearUnorderedAccessViewFloat(data.aoUavGpuHandle, data.clearUavCpuHandle, data.aoOutputResource, clearColor, 0, nullptr);

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = data.shaderBindingTable->GetDispatchRaysDesc(data.width, data.height);
    commandList->DispatchRays(&dispatchDesc);

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        data.aoOutputResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandList->ResourceBarrier(1, &barrier);
    if (data.pAoOutputState)
        *data.pAoOutputState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}
