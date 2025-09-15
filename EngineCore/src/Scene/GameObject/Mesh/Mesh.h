#pragma once
#include "Renderer/Graphics/Buffer/VertexBuffer.h"
#include "Renderer/Graphics/Buffer/IndexBuffer.h"

class Mesh
{
public:
    std::vector<IndexBuffer*> m_IndexBuffer;
    std::vector<VertexBuffer*> m_VertexBuffer;

    int m_MaterialIndex;
};

