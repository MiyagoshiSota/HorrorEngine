#include "RootSignatureBuilder.h"

RootSignatureBuilder& RootSignatureBuilder::add_constant_buffer_view(UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
    CD3DX12_ROOT_PARAMETER param;
    param.InitAsConstantBufferView(shaderRegister, registerSpace, visibility);
    m_RootParameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::add_shader_resource_view(UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
    CD3DX12_ROOT_PARAMETER param;
    param.InitAsShaderResourceView(shaderRegister, registerSpace, visibility);
    m_RootParameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::add_unordered_access_view(UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
    CD3DX12_ROOT_PARAMETER param;
    param.InitAsUnorderedAccessView(shaderRegister, registerSpace, visibility);
    m_RootParameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::add_descriptor_table(UINT rangeCount, const D3D12_DESCRIPTOR_RANGE* ranges, D3D12_SHADER_VISIBILITY visibility)
{
    CD3DX12_ROOT_PARAMETER param;
    param.InitAsDescriptorTable(rangeCount, ranges, visibility);
    m_RootParameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::add_static_sampler(const D3D12_STATIC_SAMPLER_DESC& samplerDesc)
{
    m_StaticSamplers.push_back(samplerDesc);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::set_flags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    m_Flags = flags;
    return *this;
}