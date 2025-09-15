#pragma once
#include <d3d12.h>
#include <Renderer/StandardShader/Struct/SharedStruct.h>
#include <Renderer/Graphics/Buffer/ConstantBuffer.h>
#include <Scene/GameObject/Model/Model.h>
#include <Scene/GameObject/Mesh/Mesh.h>
#include <Scene/GameObject/Material/Material.h>
#include <Renderer/StandardShader/Struct/SharedStruct.h>

class GameObjectBase
{
public:
	GameObjectBase();

	void Update();
	void Draw(ID3D12GraphicsCommandList* cmdList);

	SharedStruct::Transform m_Transform;
	std::shared_ptr<Model> m_Model;
};

