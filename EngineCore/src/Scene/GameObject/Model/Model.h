#pragma once
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/Material/Material.h"

class Model
{
public:
    Model(std::string name) : name(name)
    {
	    
    };
    ~Model()
    {
	    
    }

	std::string name;
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::vector<std::shared_ptr<Material>> m_Materials;
};