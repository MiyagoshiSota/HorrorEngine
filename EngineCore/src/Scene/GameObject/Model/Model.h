#pragma once
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/Material/Material.h"

class Model
{
public:
    std::vector<Mesh> m_Meshes;
    std::vector<std::shared_ptr<Material>> m_Materials;
};