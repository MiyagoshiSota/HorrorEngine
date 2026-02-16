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

	// --- 手持ちアイテム（GameObject インスタンス）---

	/// <summary>
	/// itemId（ModelWindow のモデル名）でアイテムを装備し、プレイヤー位置オフセットに表示する。
	/// インベントリに無い場合は false。既に持っているアイテムは Unequip される。
	/// </summary>
	bool EquipItem(const std::string& itemId);

	/// <summary>
	/// 手持ちアイテムを外し、シーンから削除する。
	/// </summary>
	void UnequipItem();

	/// <summary>
	/// 手持ちアイテムを指定位置に置く。オブジェクトはシーンに残り、インベントリから削除される。
	/// 何も持っていない場合は false。
	/// </summary>
	bool PlaceHeldItemAt(float x, float y, float z);

	/// <summary>
	/// 手持ちアイテムの Transform をプレイヤー位置＋オフセットに同期する。毎フレーム呼ぶ。
	/// </summary>
	void UpdateHeldItemTransform();

	std::shared_ptr<GameObject> GetHeldItemInstance() const { return m_heldItemInstance; }

private:
	Player() = default;
	~Player() = default;

	std::shared_ptr<GameObject> m_playerGameObject = nullptr;
	std::vector<std::string> m_inventory;

	std::shared_ptr<GameObject> m_heldItemInstance = nullptr;
	float m_holdOffsetX = 0.3f;
	float m_holdOffsetY = -0.2f;
	float m_holdOffsetZ = 0.5f;
};
