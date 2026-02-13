#include "GameObjectLoader.h"

#include <fstream>

#include "Modules/PublicConst/ConstGameObjectSaveParamPref.h"
#include "Core/Components/TriggerComponent.h"
#include "Core/Components/TriggerFactory.h"
#include "Core/Components/Reward/AddItemReward.h"
#include "Core/Components/Reward/PlaySoundReward.h"
#include "Core/Components/Reward/PrintHelloReward.h"
#include "Core/Components/Reward/SetObjectTransformReward.h"
#include "Core/Components/Reward/StartWorkReward.h"
#include "Core/Components/Trigger/ClickOnObjectCondition.h"
#include "Core/Components/Trigger/ClickWithItemCondition.h"
#include "Core/Components/Trigger/CountDownCondition.h"
#include "Core/Components/Trigger/OnGameObjectEnterCondition.h"
#include "Physics/Component/Rigidbody.h"
#include "Scene/Character/Player/PlayerController.h"
#include "Scene/GameObject/Component/ComponentFactory.h"
#include "Scene/GameObject/Component/MeshRenderer.h"

std::vector<std::shared_ptr<GameObject>> GameObjectLoader::LoadFromFile(const std::string& filePath)
{
	auto gameObjects = std::vector<std::shared_ptr<GameObject>>();
	ComponentFactory::Register<MeshRenderer>(ConstGameObjectSaveParamPref::kComponentMeshRenderer);
	ComponentFactory::Register<Rigidbody>(ConstGameObjectSaveParamPref::kComponentRigidbody);
	ComponentFactory::Register<TriggerComponent>(ConstGameObjectSaveParamPref::kComponentTrigger);
	ComponentFactory::Register<PLayerController>(ConstGameObjectSaveParamPref::kComponentPlayerController);

	std::ifstream file(filePath);
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file: " + filePath);
	}
	nlohmann::json sceneJson = nlohmann::json::parse(file);

	// コンポーネントの初期化
	// TODO:別のところに置こう。責務が違う
	ComponentsInitialize();

	for (const auto& objJson : sceneJson[ConstGameObjectSaveParamPref::kGameObjects])
	{
		auto go = std::make_shared<GameObject>();
		go->m_name = objJson[ConstGameObjectSaveParamPref::kGameObjectName];

		// positionの設定
		const auto& positionJson = objJson[ConstGameObjectSaveParamPref::kTransformPosition];
		go->SetPosition(positionJson[0], positionJson[1], positionJson[2]);

		// rotationの設定
		const auto& rotationJson = objJson[ConstGameObjectSaveParamPref::kTransformRotation];
		go->SetRotation(rotationJson[0], rotationJson[1], rotationJson[2]);

		// scaleの設定
		const auto& scaleJson = objJson[ConstGameObjectSaveParamPref::kTransformScale];
		go->SetScale(scaleJson[0], scaleJson[1], scaleJson[2]);

		// コンポーネントの設定
		if (objJson.contains("components"))
		{
			for (const auto& compJson : objJson["components"])
			{
				std::string type = compJson[ConstGameObjectSaveParamPref::kComponentType];
				if (auto new_component = ComponentFactory::create(type))
				{
					new_component->Initialize(go);
					new_component->Deserialize(compJson);
					go->components.push_back(std::move(new_component));
				}
			}
		}
		gameObjects.push_back(go);
	}
	
	return gameObjects;
}

void GameObjectLoader::ReloadFromFile(const std::string& filePath,
	std::vector<std::shared_ptr<GameObject>>& out_gameObjects)
{
	// 既存のゲームオブジェクトの状態を上書き
	out_gameObjects = LoadFromFile(filePath);
}

void GameObjectLoader::ComponentsInitialize()
{
	// Trigger Factoryの初期化
	auto& factory = TriggerFactory::GetInstance();
	// Conditionを登録
	factory.RegisterCondition<ClickOnObjectCondition>("ClickOnObjectCondition");
	factory.RegisterCondition<ClickWithItemCondition>("ClickWithItemCondition");
	factory.RegisterCondition<OnGameObjectEnterCondition>("OnGameObjectEnterCondition");
	factory.RegisterCondition<CountDownCondition>("CountDownCondition");
	// Actionを登録
	factory.RegisterAction<AddItemReward>("AddItemReward");
	factory.RegisterAction<PlaySoundReward>("PlaySoundReward");
	factory.RegisterAction<PrintHelloReward>("PrintHelloReward");
	factory.RegisterAction<SetObjectTransformReward>("SetObjectTransformReward");
	factory.RegisterAction<StartWorkReward>("StartWorkReward");
}

