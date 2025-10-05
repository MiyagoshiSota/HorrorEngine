#pragma once
#include <string>

#include "Scene/GameObject/GameObject.h"

class GameObjectLoader
{
public:
	static std::vector<std::shared_ptr<GameObject>> load_from_file(const std::string& filePath);
};

