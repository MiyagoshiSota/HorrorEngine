#pragma once
#include <cstdint>
#include <d3d12.h>
#include "Modules/ComPtr.h"

class IndexBuffer
{
public:
	IndexBuffer(size_t size, const uint32_t* pInitData = nullptr);
	~IndexBuffer()
	{
		
	}
	bool IsValid();
	D3D12_INDEX_BUFFER_VIEW View() const;

	ID3D12Resource* GetResource() const { return m_pBuffer.Get(); }
	UINT GetIndexCount() const { return static_cast<UINT>(m_View.SizeInBytes / sizeof(uint32_t)); }

private:
	bool m_IsValid = false;
	ComPtr<ID3D12Resource> m_pBuffer; // インデックスバッファ
	D3D12_INDEX_BUFFER_VIEW m_View; // インデックスバッファビュー

	IndexBuffer(const IndexBuffer&) = delete;
	void operator = (const IndexBuffer&) = delete;
};

