#pragma once
#include "Modules/ComPtr.h"
#include <string>
#include <d3dx12.h>
#include <memory>
#include <unordered_map>

class SrvDescriptorHeap;
class DescriptorHandle;

class Texture2D : public std::enable_shared_from_this<Texture2D>
{
public:
	Texture2D(std::string path);
	Texture2D(std::wstring path);
	Texture2D(ComPtr<ID3D12Resource> buffer);

	static std::shared_ptr<Texture2D> CreateWhiteTexture(); // 白の単色テクスチャを生成する(TextureManagerから使用)
	bool IsValid(); // 正常に読み込まれているかどうかを返す

	~Texture2D()
	{
		
	}

	ComPtr<ID3D12Resource> Resource(); // リソースを返す
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc(); // シェーダーリソースビューの設定を返す

private:
	bool m_IsValid; // 正常に読み込まれているか
	ComPtr<ID3D12Resource> m_pResource; // リソース
	bool Load(std::string& path);
	bool Load(std::wstring& path);

	static ComPtr < ID3D12Resource > GetDefaultResource(size_t width, size_t height);

	Texture2D(const Texture2D&) = delete;
	void operator = (const Texture2D&) = delete;
};

