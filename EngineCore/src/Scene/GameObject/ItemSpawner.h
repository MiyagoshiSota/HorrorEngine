#pragma once

#include <memory>
#include <string>

class GameObject;

/// <summary>
/// ModelWindow（g_ModelLoader）に登録されたモデル名を itemId として、
/// そのモデルを持つ GameObject を生成してシーンに追加する。
/// </summary>
class ItemSpawner
{
public:
	/// <summary>
	/// modelName（itemId）に対応するモデルで GameObject を生成し、シーンに追加する。
	/// Rigidbody は付けない（持っている間は物理演算しない）。
	/// </summary>
	/// <param name="modelName">models.json / ModelLoader に登録されているモデル名（itemId）</param>
	/// <returns>生成した GameObject。モデルが存在しない場合は nullptr</returns>
	static std::shared_ptr<GameObject> SpawnItemGameObject(const std::string& modelName);
};
