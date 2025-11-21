#pragma once

#include <d3d12.h>
#include <wrl.h> // ComPtrを使うため
#include <string>

// d3dx12.h のヘルパー構造体を使うと便利
#include <d3dx12.h>

#include "ITargetBase.h"

class RenderTarget: public ITargetBase
{
public:
	RenderTarget() = default;

    // 生成
    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        UINT16 array_size,
        UINT16 mip_levels,
        UINT sample_count,
        UINT sample_quality,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    ) override;

	// DSV用のCreateは使わない
	void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT resourceFormat,
        DXGI_FORMAT dsvFormat,
        DXGI_FORMAT srvFormat,
        UINT16 array_size,
        UINT16 mip_levels,
        UINT sample_count,
        UINT sample_quality,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    ) override{};
    
private:
   DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
};
