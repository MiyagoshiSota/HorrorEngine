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
    // レンダーターゲットを生成する
    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT format,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    ) override;

    void Create(
        ID3D12Device* pDevice,
        UINT width,
        UINT height,
        DXGI_FORMAT resourceFormat,
        DXGI_FORMAT dsvFormat,
        DXGI_FORMAT srvFormat,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle,
        std::shared_ptr<DescriptorHandle> srvHandle
    ) override {};
    
private:
   DXGI_FORMAT m_Format;
};
