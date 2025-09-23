#pragma once
#include "Renderer/Graphics/Buffer/VertexBuffer.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"
#include <memory>
#include <vector>

class Mesh
{
public:
    std::vector<std::unique_ptr<IndexBuffer>> m_IndexBuffers;
    std::vector<std::unique_ptr<VertexBuffer>> m_VertexBuffer;

    int m_MaterialIndex;
};
