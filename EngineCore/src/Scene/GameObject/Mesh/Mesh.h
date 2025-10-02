#pragma once
#include "Renderer/Graphics/Buffer/VertexBuffer.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include <memory>
#include <vector>

class Mesh
{
public:
    std::vector<std::shared_ptr<IndexBuffer>> m_IndexBuffers;
    std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffer;

    ~Mesh()
    {

    }

    int m_MaterialIndex;
};
