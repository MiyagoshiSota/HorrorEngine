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

	void start() override {}
	void update(float deltaTime) override {
	}

	void deserialize(const nlohmann::json& jsonData,std::shared_ptr<GameObject> obj) override {

		if (!jsonData.contains("model_name")) return;

		model_name = jsonData["model_name"].get<std::string>();
		
		// モデルのロード
		const auto model = g_ModelLoader->GetModel(model_name);

		// 既にロードされているなら何もしない
		if (model)
		{
			// GameObjectにモデルをセット
			obj->set_model(model);
		}else
		{
			printf("存在しないモデルパス:%s\n",model_name.c_str());
		} 
	}

	std::string get_type() override {;
		return "MeshRenderer";
	}

	void on_gui() override {
		ImGui::Text("MeshRenderer Component");
		ImGui::Separator();
		ImGui::Text("Model Name: %s", model_name.c_str());
	}

public:
	std::string model_name;

private:

};

