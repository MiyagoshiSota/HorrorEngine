#pragma once
#include "Scene/GameObject/GameObjectBase.h"

class SceneResourceManager
{
public:
	/// <summary>
	/// GameObjectのリソースを読み込んでバッファを確保
	/// </summary>
	/// <param name="obj"></param>
	void InitializeGpuResourcesFor(std::shared_ptr<GameObjectBase> obj);

private:
	void CreateVertexBuffer(std::shared_ptr<Model> meshes);
	void CreateIndexBuffer(std::shared_ptr<Model> model);
	void ReadMaterial(std::shared_ptr<Model> model);
};
