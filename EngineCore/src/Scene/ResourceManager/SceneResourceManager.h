#pragma once
#include "Scene/GameObject/GameObjectBase.h"

class SceneResourceManager
{
public:
	/// <summary>
	/// GameObjectのリソースを読み込んでバッファを確保
	/// </summary>
	/// <param name="obj"></param>
	void InitializeGpuResourcesFor(GameObjectBase& obj);

private:
	void CreateVertexBuffer(std::vector<Mesh> meshes);
	void CreateIndexBuffer(GameObjectBase& obj);
	void ReadMaterial(GameObjectBase& obj);
};
