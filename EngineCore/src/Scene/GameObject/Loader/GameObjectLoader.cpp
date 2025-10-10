#include "GameObjectLoader.h"

#include <fstream>

#include "Physics/Component/Rigidbody.h"
#include "Scene/GameObject/Component/ComponentFactory.h"
#include "Scene/GameObject/Component/MeshRenderer.h"

std::vector<std::shared_ptr<GameObject>> GameObjectLoader::load_from_file(const std::string& filePath)
{
	auto gameObjects = std::vector<std::shared_ptr<GameObject>>();
	ComponentFactory::Register<MeshRenderer>("MeshRenderer");
	ComponentFactory::Register<Rigidbody>("Rigidbody");

	std::ifstream file(filePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file: " + filePath);
	}
	nlohmann::json sceneJson = nlohmann::json::parse(file);

	for (const auto& objJson : sceneJson["gameObjects"])
	{
		auto go = std::make_shared<GameObject>();
		go->name = objJson["name"];

		// positionの設定
		const auto& positionJson = objJson["position"];
		go->set_position(positionJson[0], positionJson[1], positionJson[2]);

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
