#pragma once

#include <memory>
#include <string>
#include <vector>

class GameObject;

/// <summary>
/// 絶対的なプレイヤーを表すシングルトン。
/// アイテムリスト（インベントリ）を保持し、絶対的なプレイヤー用 GameObject をスポーンして参照する。
/// Camera は持たない。
/// </summary>
class Player
{
public:
	static Player& GetInstance();

	Player(const Player&) = delete;
	Player& operator=(const Player&) = delete;

	// --- プレイヤー GameObject（絶対 1 体）---

	/// <summary>
	/// プレイヤー用 GameObject を生成し、シーンに追加して参照を保持する。
	/// 既に存在する場合は何もしない（false を返す）。
	/// </summary>
	/// <returns>新規スポーンした場合 true、既に存在する場合 false</returns>
	bool Spawn();

	/// <summary>
	/// 外部で生成した GameObject をプレイヤーとして登録する。
	/// </summary>
	void SetPlayerGameObject(std::shared_ptr<GameObject> go);

	std::shared_ptr<GameObject> GetPlayerGameObject() const { return m_playerGameObject; }

	/// <summary>
	/// プレイヤー GameObject が存在するか。
	/// </summary>
	bool HasPlayerGameObject() const { return m_playerGameObject != nullptr; }

	// --- アイテムリスト（インベントリ）---

	void AddItem(const std::string& itemId);
	bool HasItem(const std::string& itemId) const;
	bool RemoveItem(const std::string& itemId);
	void ClearInventory();

	const std::vector<std::string>& GetInventory() const { return m_inventory; }

private:
	Player() = default;
	~Player() = default;

	std::shared_ptr<GameObject> m_playerGameObject = nullptr;
	std::vector<std::string> m_inventory;
};
