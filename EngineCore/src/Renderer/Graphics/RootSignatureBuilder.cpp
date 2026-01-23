#include "RootSignatureBuilder.h"

RootSignatureBuilder& RootSignatureBuilder::AddConstantBufferView(UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
    CD3DX12_ROOT_PARAMETER param;
    param.InitAsConstantBufferView(shaderRegister, registerSpace, visibility);
    m_RootParameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddShaderResourceView(UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
    CD3DX12_ROOT_PARAMETER param;
    param.InitAsShaderResourceView(shaderRegister, registerSpace, visibility);
    m_RootParameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddUnorderedAccessView(UINT shaderRegister, UINT registerSpace, D3D12_SHADER_VISIBILITY visibility)
{
    CD3DX12_ROOT_PARAMETER param;
    param.InitAsUnorderedAccessView(shaderRegister, registerSpace, visibility);
    m_RootParameters.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddDescriptorTable(UINT rangeCount, const D3D12_DESCRIPTOR_RANGE* ranges, D3D12_SHADER_VISIBILITY visibility)
{
	auto& storedRanges = m_RangeStorage.emplace_back();

    storedRanges.resize(rangeCount);
    for (UINT i = 0; i < rangeCount; ++i)
    {
        storedRanges[i] = ranges[i];
    }

    CD3DX12_ROOT_PARAMETER param;
    param.InitAsDescriptorTable(rangeCount, storedRanges.data(), visibility);
    m_RootParameters.push_back(param);

    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& samplerDesc)
{
    m_StaticSamplers.push_back(samplerDesc);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    m_Flags = flags;
    return *this;
}