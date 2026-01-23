#pragma once
#include <string>

#include "Scene/GameObject/GameObject.h"

class GameObjectLoader
{
public:
	static std::vector<std::shared_ptr<GameObject>> LoadFromFile(const std::string& filePath);
	static void ReloadFromFile(const std::string& filePath, std::vector<std::shared_ptr<GameObject>>& out_gameObjects);
	static void ComponentsInitialize();
};

