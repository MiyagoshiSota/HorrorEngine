#pragma once
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/Material/Material.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Renderer/Graphics/DescriptorHeap//DescriptorHeap.h"

class Model
{
public:
    Model();
    ~Model()
    {
	    
    }

    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::vector<std::shared_ptr<Material>> m_Materials;
    std::vector<SharedStruct::Mesh> m_InputMesh;
};