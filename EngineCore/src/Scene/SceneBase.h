#pragma 
#include "Scene/Camera/SceneCamera.h"
#include "Scene/Renderer/SceneRenderer.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include "Scene/GameObject/GameObjectBase.h"

class SceneBase
{
public:
	bool Init(); // 初期化
	void Update(); // 更新処理
	void Draw(); // 描画処理

private:
	std::unique_ptr<SceneCamera> m_Camera;
	std::unique_ptr<SceneRenderer> m_Renderer;
	std::unique_ptr<SceneResourceManager> m_SceneResourceManager;
	GameObjectBase m_GameObjects[100]; // 100オブジェクトが上限
};

extern SceneBase* g_Scene;