#pragma 
#include <d3d12.h>
#include "Scene/Camera/SceneCamera.h"
#include "Scene/Renderer/SceneRenderer.h"
#include "Scene/ResourceManager/SceneResourceManager.h"
#include "Scene/GameObject/GameObjectBase.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"

class SceneBase
{
public:
	bool Init(); // 初期化
	void Update(); // 更新処理
	void Draw(); // 描画処理

private:
	void SetConstantBuffer();

	std::unique_ptr<SceneCamera> m_Camera;
	std::unique_ptr<SceneRenderer> m_Renderer;
	std::unique_ptr<SceneResourceManager> m_SceneResourceManager;
	ConstantBuffer* constantBuffer[Engine::FRAME_BUFFER_COUNT];
	std::vector<std::shared_ptr <GameObjectBase>> m_GameObjects; // 100オブジェクトが上限
};

extern SceneBase* g_Scene;