#pragma once
#include "Scene/GameObject/GameObject.h"

class SceneResourceManager
{
public:
	/// <summary>
	/// GameObjectのリソースを読み込んでバッファを確保
	/// </summary>
	/// <param name="obj"></param>
	void initialize_gpu_resources_for(std::shared_ptr<GameObject> obj);

private:
	void create_vertex_buffer(std::shared_ptr<Model> meshes);
	void create_index_buffer(std::shared_ptr<Model> model);
	void read_material(std::shared_ptr<Model> model);
};
