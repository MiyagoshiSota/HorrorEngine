#pragma once
#include <nlohmann/adl_serializer.hpp>
#include <optional>
#include <vector>
#include <DirectXMath.h>

#include "Core/App.h"
#include "Modules/PublicConst/ConstGameObjectSaveParamPref.h"
#include "Renderer/Assimp/AssimpLoader.h"
#include "Scene/GameObject/Component/Component.h"
#include "Scene/GameObject/Model/Model.h"
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

		// MaterialColorOverride の復元
		if (jsonData.contains(ConstGameObjectSaveParamPref::kMeshRendererMaterialColorOverrides) &&
			jsonData[ConstGameObjectSaveParamPref::kMeshRendererMaterialColorOverrides].is_array())
		{
			const auto& arr = jsonData[ConstGameObjectSaveParamPref::kMeshRendererMaterialColorOverrides];
			for (size_t i = 0; i < arr.size(); ++i)
			{
				if (!arr[i].is_null() && arr[i].is_array() && arr[i].size() >= 4)
				{
					DirectX::XMFLOAT4 color = {
						arr[i][0].get<float>(),
						arr[i][1].get<float>(),
						arr[i][2].get<float>(),
						arr[i][3].get<float>()
					};
					SetMaterialColorOverride(i, color);
				}
			}
		}
	}

	std::string GetType() override {;
		return "MeshRenderer";
	}

#ifndef BUILD_STANDALONE
	void OnGui() override {
		ImGui::Text("MeshRenderer Component");
		ImGui::Separator();
		ImGui::Text("Model Name: %s", model_name.c_str());

		if (model != nullptr && !model->m_Materials.empty())
		{
			ImGui::Separator();
			ImGui::Text("Material Color Overrides");
			for (size_t i = 0; i < model->m_Materials.size(); ++i)
			{
				DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
				const bool hasOverride = GetMaterialColorOverride(i, color);
				if (!hasOverride)
					color = model->m_Materials[i]->GetColor();

				char label[64];
				snprintf(label, sizeof(label), "Material %zu", i);
				if (ImGui::ColorEdit4(label, &color.x))
					SetMaterialColorOverride(i, color);
				if (hasOverride)
				{
					char clearLabel[64];
					snprintf(clearLabel, sizeof(clearLabel), "Clear##%zu", i);
					if (ImGui::Button(clearLabel))
						ClearMaterialColorOverride(i);
				}
			}
		}
	}
#endif // BUILD_STANDALONE

	/// 指定マテリアルインデックスの色をインスタンス用にオーバーライドする。描画時はこの色が優先される。
	void SetMaterialColorOverride(size_t materialIndex, DirectX::XMFLOAT4 color)
	{
		if (m_materialColorOverrides.size() <= materialIndex)
			m_materialColorOverrides.resize(materialIndex + 1);
		m_materialColorOverrides[materialIndex] = color;
	}
	/// オーバーライドがあれば outColor に設定し true を返す。なければ false。
	bool GetMaterialColorOverride(size_t materialIndex, DirectX::XMFLOAT4& outColor) const
	{
		if (materialIndex >= m_materialColorOverrides.size())
			return false;
		const auto& opt = m_materialColorOverrides[materialIndex];
		if (!opt.has_value())
			return false;
		outColor = *opt;
		return true;
	}
	/// 指定マテリアルインデックスの色オーバーライドを解除する。
	void ClearMaterialColorOverride(size_t materialIndex)
	{
		if (materialIndex >= m_materialColorOverrides.size())
			return;
		m_materialColorOverrides[materialIndex] = std::nullopt;
	}

	const std::vector<std::optional<DirectX::XMFLOAT4>>& GetMaterialColorOverrides() const
	{
		return m_materialColorOverrides;
	}

public:
	std::string model_name;
	std::shared_ptr<Model> model = nullptr;

private:
	std::vector<std::optional<DirectX::XMFLOAT4>> m_materialColorOverrides;
};

