#pragma once
#include "Scene/GameObject/GameObjectBase.h"
#include "Renderer/Graphics/RootSignature.h"
#include "Renderer/Graphics/PipelineState.h"
#include "Renderer/Engine.h"
#include <d3d12.h>

class SceneRenderer
{
public:
	SceneRenderer(std::wstring vsfilePath, std::wstring psfilePath); // rendererの作成時に色々のInitializeをする
	void BeginFrame();
	void UpdateFrame();
	void EndFrame();
	void DrawGameObject(ID3D12GraphicsCommandList* commandList, std::shared_ptr<GameObjectBase> obj);

	ID3D12RootSignature* GetRootSignature() { return rootSignature->Get(); }
	ID3D12PipelineState* GetPipelineState() { return pipelineState->Get(); }

private:
	RootSignature* rootSignature;
	PipelineState* pipelineState;
};

