#pragma once
#include <nlohmann/adl_serializer.hpp>

#include "Core/App.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/GameObject/Component/Component.h"
#include "Modules/Other/engineString.h"

class MeshRenderer : public Component
{
public:
	MeshRenderer() = default;
	~MeshRenderer() override = default;

	void start() override {
	}
	void update(float deltaTime) override {
	}

	void deserialize(const nlohmann::json& jsonData,std::shared_ptr<GameObject> obj) override {

		if (jsonData.contains("model_path"))
		{
			model_path = jsonData["model_path"];
		}
		else
		{
			return;
		}

		// モデルのロード
		auto model = g_Scene->get_models()[model_path];

		// 既にロードされているなら何もしない
		if (model)
		{
			// GameObjectにモデルをセット
			obj->set_model(model);
			return;
		}; 

		// ロードされていないなら新しくロードする
		model = std::make_shared<Model>();

		// GameObjectにモデルをセット
		obj->set_model(model);

		auto path2wst = engine_string::to_wstring(model_path);
		ImportSettings importSetting =
		{
			path2wst.c_str(),
			model->m_InputMesh,
			false,
			true
		};

		AssimpLoader loader;
		if (!loader.Load(importSetting))
		{
			printf("モデルのロードに失敗:%s\n", model_path.c_str());;
			return ;
		}
	}

public:
	std::string model_path;

private:

};

