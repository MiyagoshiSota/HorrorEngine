#pragma once
#include <nlohmann/adl_serializer.hpp>

#include "Core/App.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/GameObject/Component/Component.h"
#include "Modules/Other/EngineString.h"

class MeshRenderer : public Component
{
public:
	MeshRenderer() = default;
	~MeshRenderer() override = default;

	void Initialize(std::shared_ptr<GameObject> game_object) override
	{
		Component::Initialize(game_object);
	};

	void start() override {}
	void update(float deltaTime) override {
	}

	void Deserialize(const nlohmann::json& jsonData) override {

		if (!jsonData.contains("model_name")) return;

		model_name = jsonData["model_name"].get<std::string>();
		
		// モデルのロード
		model = g_ModelLoader->GetModel(model_name);

		// 既にロードされているなら何もしない
		if (model == nullptr)
		{
			printf("存在しないモデルパス:%s\n",model_name.c_str());
		} 
	}

	std::string GetType() override {;
		return "MeshRenderer";
	}

	void OnGui() override {
		ImGui::Text("MeshRenderer Component");
		ImGui::Separator();
		ImGui::Text("Model Name: %s", model_name.c_str());
	}

public:
	std::string model_name;
	std::shared_ptr<Model> model = nullptr;
};

