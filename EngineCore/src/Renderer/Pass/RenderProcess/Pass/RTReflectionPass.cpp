#include "RTReflectionPass.h"
#include "Renderer/Engine.h"
#include "Scene/RayTracing/RayTracedReflectionManager.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include <d3dx12.h>
#include <DirectXMath.h>

void RTReflectionPass::Execute(RenderContext& context)
{
    if (!m_enabled)
        return;

    const RayTracedReflectionRenderData& data = context.GetRayTracedReflectionData();
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

    if (data.reflectionManager)
        data.reflectionManager->EnsureGeometryDescriptorsAndSBT(device5.Get(), data.asManager, context.GameObjects);

    auto worldPosRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);
    auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
    auto materialRT = context.GetRenderTarget(ConstRenderPref::GBufferMaterial);
    if (!worldPosRT || !normalRT || !worldPosRT->GetSRVHandle() || !normalRT->GetSRVHandle())
        return;

    ID3D12DescriptorHeap* heaps[] = { data.descriptorHeap };
    commandList->SetDescriptorHeaps(1, heaps);
    { static int s_logCount = 0; if (s_logCount < 2) { printf("[RTReflection/Pass] SetDescriptorHeaps: heap=%p (expect same as [RTReflection/Heap] geometry descriptors)\n", data.descriptorHeap); ++s_logCount; } }

    D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrvDesc = {};
    tlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    tlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    tlasSrvDesc.RaytracingAccelerationStructure.Location = data.asManager->GetTopLevelAS()->GetGpuAddress();
    device->CreateShaderResourceView(nullptr, &tlasSrvDesc, data.tlasSrvCpuHandle);

    RayTracedReflectionConstants constants = {};
    DirectX::XMStoreFloat3(&constants.cameraPosition, context.Camera->GetEyePos());
    constants.bias = m_bias;
    constants.maxDistance = m_maxDistance;
    constants.reflectionIntensity = m_reflectionIntensity;
    constants.roughnessThreshold = m_roughnessThreshold;
    constants.fresnelF0 = m_fresnelF0;
    constants.frameIndex = g_Engine->CurrentBackBufferIndex();
    constants.skyColor = DirectX::XMFLOAT3(0.3f, 0.35f, 0.45f);
    const size_t vertexSize = sizeof(SharedStruct::Vertex);
    constants.vertexStrideBytes = static_cast<UINT>(vertexSize);
    { static int s_sizeLog = 0; if (s_sizeLog < 1) { printf("[RTReflection] sizeof(SharedStruct::Vertex) = %zu bytes\n", vertexSize); ++s_sizeLog; } }
    context.UpdateRayTracedReflectionConstants(constants, constants.frameIndex);

    commandList->SetPipelineState1(data.pipelineState->GetStateObject());
    ID3D12RootSignature* globalRootSig = data.pipelineState->GetGlobalRootSignature();
    if (globalRootSig)
        commandList->SetComputeRootSignature(globalRootSig);

    commandList->SetComputeRootDescriptorTable(0, data.tlasSrvGpuHandle);
    commandList->SetComputeRootDescriptorTable(1, worldPosRT->GetSRVHandle()->gpuHandle);
    commandList->SetComputeRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
    if (materialRT && materialRT->GetSRVHandle())
        commandList->SetComputeRootDescriptorTable(3, materialRT->GetSRVHandle()->gpuHandle);
    else
        commandList->SetComputeRootDescriptorTable(3, normalRT->GetSRVHandle()->gpuHandle);
    commandList->SetComputeRootDescriptorTable(4, data.reflectionUavGpuHandle);
    if (data.constantBuffer)
        commandList->SetComputeRootConstantBufferView(5, data.constantBuffer->GetGPUVirtualAddress());

    if (data.pReflectionOutputState && *data.pReflectionOutputState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            data.reflectionOutputResource,
            *data.pReflectionOutputState,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );
        commandList->ResourceBarrier(1, &barrier);
        *data.pReflectionOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (data.clearUavCpuHandle.ptr != 0)
        commandList->ClearUnorderedAccessViewFloat(data.reflectionUavGpuHandle, data.clearUavCpuHandle, data.reflectionOutputResource, clearColor, 0, nullptr);

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = data.shaderBindingTable->GetDispatchRaysDesc(data.width, data.height);
    commandList->DispatchRays(&dispatchDesc);

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        data.reflectionOutputResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandList->ResourceBarrier(1, &barrier);
    if (data.pReflectionOutputState)
        *data.pReflectionOutputState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}
