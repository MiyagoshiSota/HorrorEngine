#include "GameObjectLoader.h"

#include <fstream>

#include "Core/Components/TriggerComponent.h"
#include "Core/Components/TriggerFactory.h"
#include "Core/Components/Action/PlaySoundAction.h"
#include "Core/Components/Trigger/OnGameObjectEnterCondition.h"
#include "Physics/Component/Rigidbody.h"
#include "Scene/GameObject/Component/ComponentFactory.h"
#include "Scene/GameObject/Component/MeshRenderer.h"

std::vector<std::shared_ptr<GameObject>> GameObjectLoader::load_from_file(const std::string& filePath)
{
	auto gameObjects = std::vector<std::shared_ptr<GameObject>>();
	ComponentFactory::Register<MeshRenderer>("MeshRenderer");
	ComponentFactory::Register<Rigidbody>("Rigidbody");
	ComponentFactory::Register<TriggerComponent>("Trigger");

	std::ifstream file(filePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file: " + filePath);
	}
	nlohmann::json sceneJson = nlohmann::json::parse(file);

	// コンポーネントの初期化
	// TODO:別のところに置こう。責務が違う
	components_initialize();

	for (const auto& objJson : sceneJson["gameObjects"])
	{
		auto go = std::make_shared<GameObject>();
		go->name = objJson["name"];

		// positionの設定
		const auto& positionJson = objJson["position"];
		go->set_position(positionJson[0], positionJson[1], positionJson[2]);

		// rotationの設定
		const auto& rotationJson = objJson["rotation"];
		go->set_rotation(rotationJson[0], rotationJson[1], rotationJson[2]);

		// scaleの設定
		const auto& scaleJson = objJson["scale"];
		go->set_scale(scaleJson[0], scaleJson[1], scaleJson[2]);

		// コンポーネントの設定
		if (objJson.contains("components"))
		{
			for (const auto& compJson : objJson["components"])
			{
				std::string type = compJson["type"];
				if (auto new_component = ComponentFactory::create(type))
				{
					new_component->gameObject = go;
					new_component->deserialize(compJson,go);
					go->components.push_back(std::move(new_component));
				}
			}
		}
		gameObjects.push_back(go);
	}
	
	return gameObjects;
}

void GameObjectLoader::components_initialize()
{
	// Trigger Factoryの初期化
	auto& factory = TriggerFactory::GetInstance();
	// Conditionを登録
	factory.RegisterCondition<OnGameObjectEnterCondition>("OnGameObjectEnterCondition");
	// Actionを登録
	factory.RegisterAction<PlaySoundAction>("PlaySoundAction");
}

