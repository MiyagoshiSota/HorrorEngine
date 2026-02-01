#pragma once
#include <d3d12.h>
#include "Modules/ComPtr.h"

class VertexBuffer
{
public:
	VertexBuffer(size_t size, size_t stride, const void* pInitData); // コンストラクタでバッファを作成
	~VertexBuffer()
	{
		
	}
	D3D12_VERTEX_BUFFER_VIEW View() const; // 頂点バッファビューを取得
	bool IsValid(); // バッファの作成に成功したかを取得
	void CopyData(size_t size, const void* pInitData);

	// DXR用のメソッド
	ID3D12Resource* GetResource() const { return m_pBuffer.Get(); }
	UINT GetVertexCount() const { return static_cast<UINT>(m_View.SizeInBytes / m_View.StrideInBytes); }
	UINT GetStride() const { return static_cast<UINT>(m_View.StrideInBytes); }

private:
	bool m_IsValid = false; // バッファの作成に成功したかを取得
	ComPtr<ID3D12Resource> m_pBuffer = nullptr; // バッファ
	D3D12_VERTEX_BUFFER_VIEW m_View = {}; // 頂点バッファビュー

	VertexBuffer(const VertexBuffer&) = delete;
	void operator = (const VertexBuffer&) = delete;
};

