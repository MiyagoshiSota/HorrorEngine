#pragma once
#include <memory>

#include "Core/App.h"
#include "Scene/GameObject/GameObject.h"

class GameObjectFinder
{
public:
	/// <summary>
	/// GameObjectの名前から検索して返す
	/// </summary>
	/// <param name="name"></param>
	/// <returns></returns>
	static std::shared_ptr<GameObject> FindGameObjectsByName(const std::string& name)
	{
		if (g_Scene)
		{
			for (const auto& obj : g_Scene->GetGameObjects())
			{
				if (obj->GetName() == name)
				{
					return obj;
				}
			}
		}
		return nullptr;
	}
};
