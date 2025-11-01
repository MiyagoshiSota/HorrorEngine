#pragma once
#include <memory> // std::shared_ptr

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
	
	static SceneResourceManager &GetInstance() {
		static SceneResourceManager instance;
		return instance;
	}

	/// <summary>
	/// GameObjectのリソースを読み込んでバッファを確保
	/// </summary>
	/// <param name="obj"></param>
	void initialize_gpu_resources_for(std::shared_ptr<GameObject> obj);
	
	void create_vertex_buffer(std::shared_ptr<Model> meshes);
	void create_index_buffer(std::shared_ptr<Model> model);
	void read_material(std::shared_ptr<Model> model);
	void create_mesh_classes(std::shared_ptr<Model> meshes);
};