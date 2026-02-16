#include "Player.h"
#include "Core/App.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/ItemSpawner.h"
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
	EquipItem(itemId);
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

bool Player::EquipItem(const std::string& itemId)
{
	if (!HasItem(itemId))
		return false;

	if (m_heldItemInstance)
		UnequipItem();

	auto go = ItemSpawner::SpawnItemGameObject(itemId);
	if (!go)
		return false;

	m_heldItemInstance = go;
	return true;
}

void Player::UnequipItem()
{
	if (!m_heldItemInstance)
		return;

	if (g_Scene)
		g_Scene->RemoveGameObject(m_heldItemInstance);
	m_heldItemInstance = nullptr;
}

bool Player::PlaceHeldItemAt(float x, float y, float z)
{
	if (!m_heldItemInstance)
		return false;

	const std::string& name = m_heldItemInstance->GetName();
	const std::string kPrefix = "HeldItem_";
	std::string itemId;
	if (name.size() > kPrefix.size() && name.compare(0, kPrefix.size(), kPrefix) == 0)
		itemId = name.substr(kPrefix.size());
	else
		itemId = name;

	m_heldItemInstance->SetPosition(x, y, z);
	RemoveItem(itemId);
	m_heldItemInstance = nullptr;
	return true;
}

void Player::UpdateHeldItemTransform()
{
	if (!m_heldItemInstance || !m_playerGameObject)
		return;

	const auto pos = m_playerGameObject->GetPosition();
	m_heldItemInstance->SetPosition(
		pos.x + m_holdOffsetX,
		pos.y + m_holdOffsetY,
		pos.z + m_holdOffsetZ
	);
}
