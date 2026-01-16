#include "GameObjectLoader.h"

#include <fstream>

#include "Core/Components/TriggerComponent.h"
#include "Core/Components/TriggerFactory.h"
#include "Core/Components/Reward/PlaySoundReward.h"
#include "Core/Components/Reward/PrintHelloReward.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Core/Components/Trigger/CountDownCondition.h"
#include "Core/Components/Trigger/OnGameObjectEnterCondition.h"
#include "Physics/Component/Rigidbody.h"
#include "Scene/Character/Player/PlayerController.h"
#include "Scene/GameObject/Component/ComponentFactory.h"
#include "Scene/GameObject/Component/MeshRenderer.h"

std::vector<std::shared_ptr<GameObject>> GameObjectLoader::load_from_file(const std::string& filePath)
{
	auto gameObjects = std::vector<std::shared_ptr<GameObject>>();
	ComponentFactory::Register<MeshRenderer>(const_gameobject_save_param_pref::ComponentMeshRenderer);
	ComponentFactory::Register<Rigidbody>(const_gameobject_save_param_pref::ComponentRigidbody);
	ComponentFactory::Register<TriggerComponent>(const_gameobject_save_param_pref::ComponentTrigger);
	ComponentFactory::Register<PLayerController>(const_gameobject_save_param_pref::ComponentPlayerController);

	std::ifstream file(filePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file: " + filePath);
	}
	nlohmann::json sceneJson = nlohmann::json::parse(file);

	// コンポーネントの初期化
	// TODO:別のところに置こう。責務が違う
	components_initialize();

	for (const auto& objJson : sceneJson[const_gameobject_save_param_pref::GameObjects])
	{
		auto go = std::make_shared<GameObject>();
		go->name = objJson[const_gameobject_save_param_pref::GameObjectName];

		// positionの設定
		const auto& positionJson = objJson[const_gameobject_save_param_pref::TransformPosition];
		go->set_position(positionJson[0], positionJson[1], positionJson[2]);

		// rotationの設定
		const auto& rotationJson = objJson[const_gameobject_save_param_pref::TransformRotation];
		go->set_rotation(rotationJson[0], rotationJson[1], rotationJson[2]);

		// scaleの設定
		const auto& scaleJson = objJson[const_gameobject_save_param_pref::TransformScale];
		go->set_scale(scaleJson[0], scaleJson[1], scaleJson[2]);

		// コンポーネントの設定
		if (objJson.contains("components"))
		{
			for (const auto& compJson : objJson["components"])
			{
				std::string type = compJson[const_gameobject_save_param_pref::ComponentType];
				if (auto new_component = ComponentFactory::create(type))
				{
					new_component->initialize(go);
					new_component->deserialize(compJson);
					go->components.push_back(std::move(new_component));
				}
			}
		}
		gameObjects.push_back(go);
	}
	
	return gameObjects;
}

void GameObjectLoader::re_load_from_file(const std::string& filePath,
	std::vector<std::shared_ptr<GameObject>>& out_gameObjects)
{
	// 既存のゲームオブジェクトの状態を上書き
	out_gameObjects = load_from_file(filePath);
}

void GameObjectLoader::components_initialize()
{
	// Trigger Factoryの初期化
	auto& factory = TriggerFactory::GetInstance();
	// Conditionを登録
	factory.RegisterCondition<OnGameObjectEnterCondition>("OnGameObjectEnterCondition");
	factory.RegisterCondition<CountDownCondition>("CountDownCondition");
	// Actionを登録
	factory.RegisterAction<PlaySoundReward>("PlaySoundReward");
	factory.RegisterAction<PrintHelloReward>("PrintHelloReward");
	factory.RegisterAction<StartWorkReward>("StartWorkReward");
}

