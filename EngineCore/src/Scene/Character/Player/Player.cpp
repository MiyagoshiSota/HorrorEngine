#include "Player.h"
#include "Core/App.h"
#include "Scene/GameObject/GameObject.h"
#include <algorithm>

Player& Player::GetInstance()
{
	static Player instance;
	return instance;
}

bool Player::Spawn()
{
	if (m_playerGameObject != nullptr)
		return false;

	auto go = std::make_shared<GameObject>();
	go->m_name = "Player";

	if (g_Scene)
		g_Scene->AddGameObject(go);

	m_playerGameObject = go;
	return true;
}

void Player::SetPlayerGameObject(std::shared_ptr<GameObject> go)
{
	m_playerGameObject = std::move(go);
}

void Player::AddItem(const std::string& itemId)
{
	m_inventory.push_back(itemId);
}

bool Player::HasItem(const std::string& itemId) const
{
	for (const auto& id : m_inventory)
	{
		if (id == itemId)
			return true;
	}
	return false;
}

bool Player::RemoveItem(const std::string& itemId)
{
	const auto it = std::find(m_inventory.begin(), m_inventory.end(), itemId);
	if (it == m_inventory.end())
		return false;
	m_inventory.erase(it);
	return true;
}

void Player::ClearInventory()
{
	m_inventory.clear();
}
