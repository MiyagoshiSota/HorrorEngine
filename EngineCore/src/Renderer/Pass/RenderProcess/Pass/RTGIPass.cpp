#include "RTGIPass.h"
#include "Renderer/Engine.h"
#include "Scene/RayTracing/RayTracedGIManager.h"
#include "Renderer/RayTracing/AccelerationStructure.h"
#include "Scene/ISceneBase.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/DxHelper.h"
#include <d3dx12.h>
#include <wrl/client.h>
#include <DirectXMath.h>

#include "Core/App.h"

void RTGIPass::Execute(RenderContext& context)
{
    if (!m_enabled)
        return;

    const RayTracedGIRenderData& data = context.GetRayTracedGIData();
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

    if (data.giManager)
        data.giManager->EnsureGeometryDescriptorsAndSBT(device5.Get(), data.asManager, context.GameObjects);

    auto worldPosRT = context.GetRenderTarget(ConstRenderPref::WorldPositionBuffer);
    auto normalRT = context.GetRenderTarget(ConstRenderPref::NormalBuffer);
    auto albedoRT = context.GetRenderTarget(ConstRenderPref::GBufferAlbedo);
    if (!worldPosRT || !normalRT || !worldPosRT->GetSRVHandle() || !normalRT->GetSRVHandle())
        return;

    ID3D12DescriptorHeap* heaps[] = { data.descriptorHeap };
    commandList->SetDescriptorHeaps(1, heaps);

    D3D12_SHADER_RESOURCE_VIEW_DESC tlasSrvDesc = {};
    tlasSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    tlasSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    tlasSrvDesc.RaytracingAccelerationStructure.Location = data.asManager->GetTopLevelAS()->GetGpuAddress();
    device->CreateShaderResourceView(nullptr, &tlasSrvDesc, data.tlasSrvCpuHandle);

    RayTracedGIConstants constants = {};
    DirectX::XMStoreFloat3(&constants.cameraPosition, context.Camera->GetEyePos());
    constants.radius = m_radius;
    constants.bias = m_bias;
    constants.indirectIntensity = m_indirectIntensity;
    constants.frameIndex = g_Engine->CurrentBackBufferIndex();
    constants.numRaysPerPixel = m_numRaysPerPixel;
    constants.skyColor = DirectX::XMFLOAT3(0.3f, 0.35f, 0.45f);
    constants.vertexStrideBytes = 60u; // SharedStruct::Vertex
    context.UpdateRayTracedGIConstants(constants, constants.frameIndex);

    commandList->SetPipelineState1(data.pipelineState->GetStateObject());
    ID3D12RootSignature* globalRootSig = data.pipelineState->GetGlobalRootSignature();
    if (globalRootSig)
        commandList->SetComputeRootSignature(globalRootSig);

    commandList->SetComputeRootDescriptorTable(0, data.tlasSrvGpuHandle);
    commandList->SetComputeRootDescriptorTable(1, worldPosRT->GetSRVHandle()->gpuHandle);
    commandList->SetComputeRootDescriptorTable(2, normalRT->GetSRVHandle()->gpuHandle);
    commandList->SetComputeRootDescriptorTable(3, data.giUavGpuHandle);
    {
        D3D12_GPU_DESCRIPTOR_HANDLE albedoHandle = (albedoRT && albedoRT->GetSRVHandle())
            ? albedoRT->GetSRVHandle()->gpuHandle
            : normalRT->GetSRVHandle()->gpuHandle;
        commandList->SetComputeRootDescriptorTable(4, albedoHandle);
    }
    if (data.constantBuffer)
        commandList->SetComputeRootConstantBufferView(5, data.constantBuffer->GetGPUVirtualAddress());
    if (g_Scene && g_Scene->GetLightingManager())
        commandList->SetComputeRootConstantBufferView(6, g_Scene->GetLightingManager()->GetConstantBuffer()->GetAddress());

    if (data.pGiOutputState && *data.pGiOutputState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            data.giOutputResource,
            *data.pGiOutputState,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        );
        commandList->ResourceBarrier(1, &barrier);
        *data.pGiOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (data.clearUavCpuHandle.ptr != 0)
        commandList->ClearUnorderedAccessViewFloat(data.giUavGpuHandle, data.clearUavCpuHandle, data.giOutputResource, clearColor, 0, nullptr);

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = data.shaderBindingTable->GetDispatchRaysDesc(data.width, data.height);
    commandList->DispatchRays(&dispatchDesc);

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        data.giOutputResource,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    commandList->ResourceBarrier(1, &barrier);
    if (data.pGiOutputState)
        *data.pGiOutputState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}
