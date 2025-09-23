#pragma once
class SampleScene
{
public:
	bool Init(); // 初期化
	void Update(); // 更新処理
	void Draw(); // 描画処理
};

extern SampleScene* g_DemoScene;