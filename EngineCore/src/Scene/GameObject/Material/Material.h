#pragma once
#include "Renderer/Texture/Texture2D.h"
#include "Modules/ComPtr.h"
#include "Renderer/Graphics/DescriptorHeap.h"

class Material
{
public:
    std::shared_ptr<Texture2D> m_Texture;
    std::unique_ptr<DescriptorHeap> m_DescriptorHeap;
    std::vector<std::shared_ptr<DescriptorHandle>> m_MaterialHandles;
};

