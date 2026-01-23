#pragma once
#include <d3d12.h>
#include <vector>
#include <d3dx12.h> // CD3DX12ヘルパーを使う

class RootSignature;

class RootSignatureBuilder
{
    // RootSignatureクラスから内部データにアクセスできるようにする
    friend class RootSignature;

public:
    RootSignatureBuilder() = default;

    // パラメータを追加するメソッド群
    RootSignatureBuilder& AddConstantBufferView(UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    RootSignatureBuilder& AddShaderResourceView(UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
    RootSignatureBuilder& AddUnorderedAccessView(UINT shaderRegister, UINT registerSpace = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    // ディスクリプタテーブルを追加するメソッド
    // rangeCount: レンジの数, ranges: D3D12_DESCRIPTOR_RANGEの配列
    RootSignatureBuilder& AddDescriptorTable(UINT rangeCount, const D3D12_DESCRIPTOR_RANGE* ranges, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    // スタティックサンプラーを追加するメソッド
    RootSignatureBuilder& AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& samplerDesc);

    // フラグを設定するメソッド
    RootSignatureBuilder& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

protected:
    // 生成時に使用する内部データ
    std::vector<CD3DX12_ROOT_PARAMETER> m_RootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers;
    D3D12_ROOT_SIGNATURE_FLAGS m_Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

private:
	std::list<std::vector<D3D12_DESCRIPTOR_RANGE>> m_RangeStorage;
};