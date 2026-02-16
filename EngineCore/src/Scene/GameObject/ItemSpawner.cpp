#include "ItemSpawner.h"
#include "Core/App.h"
#include "Modules/PublicConst/ConstGameObjectSaveParamPref.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/ComponentFactory.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include <nlohmann/json.hpp>

std::shared_ptr<GameObject> ItemSpawner::SpawnItemGameObject(const std::string& modelName)
{
	if (!g_ModelLoader || !g_Scene)
		return nullptr;

	if (g_ModelLoader->GetModel(modelName) == nullptr)
		return nullptr;

	auto go = std::make_shared<GameObject>();
	go->m_name = "HeldItem_" + modelName;

	if (ComponentFactory::get_mappings().find(ConstGameObjectSaveParamPref::kComponentMeshRenderer) == ComponentFactory::get_mappings().end())
		ComponentFactory::Register<MeshRenderer>(ConstGameObjectSaveParamPref::kComponentMeshRenderer);

	auto rendererComponent = ComponentFactory::create(ConstGameObjectSaveParamPref::kComponentMeshRenderer);
	if (!rendererComponent)
		return nullptr;

	rendererComponent->Initialize(go);
	rendererComponent->Deserialize(nlohmann::json{ {"model_name", modelName} });

	// Hack:現状はマテリアルオーバーライドのみ対応(各コンポーネントも対応する必要あり)
	auto* meshRenderer = dynamic_cast<MeshRenderer*>(rendererComponent.get());
	if (meshRenderer)
	{
		for (const auto& sceneObj : g_Scene->GetGameObjects())
		{
			if (!sceneObj || sceneObj->GetName() != modelName) continue;
			const auto* sourceRenderer = sceneObj->FindComponent<MeshRenderer>();
			if (!sourceRenderer) continue;
			const auto& overrides = sourceRenderer->GetMaterialColorOverrides();
			for (size_t i = 0; i < overrides.size(); ++i)
			{
				if (overrides[i].has_value())
					meshRenderer->SetMaterialColorOverride(i, *overrides[i]);
			}
			break;
		}
	}

	go->components.push_back(std::move(rendererComponent));

	go->Init();
	g_Scene->AddGameObject(go);

	return go;
}
