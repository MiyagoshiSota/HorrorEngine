#pragma once

class SceneBase
{
public:
	bool Init(); // 初期化

	void Update(); // 更新処理
	void Draw(); // 描画処理

private:
	std::unique
};

extern SceneBase* g_Scene;