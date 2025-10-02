#pragma once
#include "Renderer/Texture/Texture2D.h"
#include "Modules/ComPtr.h"
#include "Renderer/Graphics/DescriptorHeap/SrvDescriptorHeap.h"

class Material
{
public:
    ~Material()
    {
	    
    }
    std::shared_ptr<Texture2D> m_Texture;
    std::shared_ptr<SrvDescriptorHeap> m_DescriptorHeap;
    std::vector<std::shared_ptr<DescriptorHandle>> m_MaterialHandles;
};

