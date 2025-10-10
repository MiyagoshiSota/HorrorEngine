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
	static std::shared_ptr<GameObject> find_game_objects_by_name(const std::string& name)
	{
		if (g_Scene)
		{
			for (const auto& obj : g_Scene->get_game_objects())
			{
				if (obj->get_name() == name)
				{
					return obj;
				}
			}
		}
		return nullptr;
	}
};
