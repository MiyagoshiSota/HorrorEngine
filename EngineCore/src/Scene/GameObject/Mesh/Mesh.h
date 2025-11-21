#pragma once
#include "Renderer/Graphics/Buffer/VertexBuffer.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include <memory>
#include <vector>

#include "Renderer/StandardShader/Struct/SharedStruct.h"

class Mesh
{
public:
	/// <summary>
	/// VertexBufferの生成
	/// </summary>
	/// <param name="vssize"></param>
	/// <param name="vsdata"></param>
	/// <returns></returns>
	bool create_vertex_buffer(unsigned long long vssize,SharedStruct::Vertex* vsdata)
    {
		auto stride = sizeof(SharedStruct::Vertex);
		auto pVB = std::make_shared<VertexBuffer>(vssize, stride, vsdata);
		if (!pVB->IsValid())
		{
			printf("頂点バッファの生成に失敗\n");
			return false;
		}

		m_VertexBuffer = pVB;
		return true;
    }

	/// <summary>
	/// IndexBufferの生成
	/// </summary>
	/// <param name="ibsize"></param>
	/// <param name="ibdata"></param>
	/// <returns></returns>
	bool create_index_buffer(unsigned long long ibsize, uint32_t* ibdata)
	{
		auto pIB = std::make_shared<IndexBuffer>(ibsize, ibdata);
		if (!pIB->IsValid())
		{
			printf("インデックスバッファの生成に失敗\n");
			return false;
		}
		m_IndexBuffer = pIB;
		return true;
	}

	void set_material_index(int index) { m_MaterialIndex = index; }
	int get_material_index() const { return m_MaterialIndex; }

	std::shared_ptr<IndexBuffer> get_index_buffer(){ return m_IndexBuffer; }
	std::shared_ptr<VertexBuffer> get_vertex_buffer() { return m_VertexBuffer; }

private:
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
    std::shared_ptr<VertexBuffer> m_VertexBuffer;

    int m_MaterialIndex = 0;
};
