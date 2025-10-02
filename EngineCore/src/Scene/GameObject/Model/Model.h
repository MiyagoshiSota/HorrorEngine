#pragma once
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/Material/Material.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Renderer/Graphics/DescriptorHeap//SrvDescriptorHeap.h"

class Model
{
public:
    Model();
    ~Model()
    {
	    
    }

    std::shared_ptr<Mesh> m_Meshes;
    std::shared_ptr<Material> m_Material;
    std::vector<SharedStruct::Mesh> m_InputMesh;
};