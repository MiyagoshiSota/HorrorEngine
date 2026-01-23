#pragma once
#include <d3d12.h>
#include <memory>

#include "Modules/ComPtr.h"

class RootSignatureBuilder;
struct ID3D12RootSignature;

class RootSignature
{
public:
	bool create(ID3D12Device* device, const std::shared_ptr<RootSignatureBuilder>& builder);

	bool is_valid() const; // ルートシグネチャの生成に成功したかどうかを返す
	ID3D12RootSignature* Get() const; // ルートシグネチャを返す

private:
	bool m_IsValid = false; // ルートシグネチャの生成に成功したかどうか
	ComPtr<ID3D12RootSignature> m_pRootSignature = nullptr; // ルートシグネチャ
};

