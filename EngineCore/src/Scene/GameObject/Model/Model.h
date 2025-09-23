#pragma once
#include "Scene/GameObject/Mesh/Mesh.h"
#include "Scene/GameObject/Material/Material.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Renderer/Graphics/DescriptorHeap.h"

class Model
{
public:
    Model();

    std::unique_ptr<Mesh> m_Meshes;
    std::unique_ptr<Material> m_Material;
    std::vector<SharedStruct::Mesh> m_InputMesh;
};