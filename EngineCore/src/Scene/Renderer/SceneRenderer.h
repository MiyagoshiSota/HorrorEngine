#pragma once
#include "Scene/GameObject/GameObjectBase.h"
#include "Renderer/Graphics/RootSignature.h"
#include <d3d12.h>

class SceneRenderer
{
public:
	SceneRenderer(std::wstring vsfilePath, std::wstring psfilePath); // rendererの作成時に色々のInitializeをする
	void BeginFrame();
	void UpdateFrame();
	void EndFrame();
	void DrawGameObject(ID3D12GraphicsCommandList* commandList, GameObjectBase& obj);

private:

};

