#pragma once
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/Material/Material.h"

class Model
{
public:
    Model(std::string name) : m_name(name)
    {
	    
    };
    ~Model()
    {
	    
    }

	std::string m_name;
    std::vector<std::shared_ptr<Mesh>> m_Meshes;
    std::vector<std::shared_ptr<Material>> m_Materials;
};