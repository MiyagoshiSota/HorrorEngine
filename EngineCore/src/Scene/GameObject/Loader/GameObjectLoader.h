#pragma once
#include <string>

#include "Scene/GameObject/GameObject.h"
#include <nlohmann/json_fwd.hpp>

class GameObjectLoader
{
public:
	static std::vector<std::shared_ptr<GameObject>> LoadFromFile(const std::string& filePath);
	/// 既にパース済みのシーンJSONから GameObjects のみをロードする（Day の works は呼び出し元で処理する）
	static std::vector<std::shared_ptr<GameObject>> LoadFromJson(const nlohmann::json& sceneJson);
	static void ReloadFromFile(const std::string& filePath, std::vector<std::shared_ptr<GameObject>>& out_gameObjects);
	static void ComponentsInitialize();
};

