#include "Model.h"

Model::Model()
{
	m_Meshes = std::make_unique<Mesh>();
	m_Material = std::make_unique<Material>();
}
