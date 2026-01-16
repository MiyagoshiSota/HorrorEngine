#pragma once
#include <memory> // std::shared_ptr

#include "Renderer/StandardShader/Struct/SharedStruct.h"

// 前方宣言
class GameObject;
class Model;

class SceneResourceManager {
private:
	SceneResourceManager() = default;
	~SceneResourceManager() = default;

public:
	SceneResourceManager(const SceneResourceManager &) = delete;
	SceneResourceManager &operator=(const SceneResourceManager &) = delete;
	SceneResourceManager(SceneResourceManager &&) = delete;
	SceneResourceManager &operator=(SceneResourceManager &&) = delete;

	// シングルトンの取得
	static SceneResourceManager &GetInstance() {
		static SceneResourceManager instance;
		return instance;
	}


	// モデルデータの構造体
	struct ModelData;

	/// <summary>
	/// ModelのGPUリソースを作成する
	/// </summary>
	/// <param name="origin_data"></param>
	/// <param name="model"></param>
	void initialize_gpu_resources_for(std::vector<SharedStruct::Mesh> origin_data, std::shared_ptr<Model> model);
	
	void create_vertex_buffer(ModelData meshes);
	void create_index_buffer(ModelData model);
	void read_material(ModelData model);
	void create_mesh_classes(ModelData meshes);
};