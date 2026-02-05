#include "RayTracedAOTarget.h"

void RayTracedAOTarget::SetResource(ComPtr<ID3D12Resource> resource)
{
    m_pResource = resource;
    m_CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    m_hRtv = {};
    m_hDsv = {};
    m_Width = 0;
    m_Height = 0;
    if (resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        m_Width = static_cast<UINT>(desc.Width);
        m_Height = desc.Height;
    }
}

void RayTracedAOTarget::SetSrv(std::shared_ptr<DescriptorHandle> srv)
{
    m_hSrv = srv;
}
