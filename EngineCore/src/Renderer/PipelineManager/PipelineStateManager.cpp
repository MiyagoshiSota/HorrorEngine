#include "PipelineStateManager.h"

#include <utility>

#include "Renderer/Engine.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"

void PipelineStateManager::new_create(std::wstring vsfilePath, std::wstring psfilePath,bool isInputLayout,bool isDepthFormat ,const std::shared_ptr<RootSignatureBuilder>& builder, const std::string& name)
{
	auto rootSignature = get_root_signature(name);
	auto pipelineState = get_pipeline_state(name);

	// 作成済みなら何もしない
	if (rootSignature != nullptr && pipelineState != nullptr)
	{
		return;
	}

	rootSignature = std::make_shared<RootSignature>();
	rootSignature->create(g_Engine->Device(),builder);
	if (!rootSignature->is_valid())
	{
		printf("ルートシグネチャの生成に失敗");
		return;
	}

	pipelineState = std::make_shared<PipelineState>();

	// Inputレイアウトを設定
	if (isInputLayout) pipelineState->SetInputLayout(SharedStruct::Vertex::InputLayout);

	pipelineState->SetRootSignature(rootSignature->get());
	pipelineState->SetVS(vsfilePath);
	pipelineState->SetPS(psfilePath);

	pipelineState->SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);

	// 深度ステンシルのフォーマットを設定
	if (isDepthFormat) pipelineState->SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
	else  pipelineState->SetDepthStencilFormat(DXGI_FORMAT_UNKNOWN);


	pipelineState->Create();
	if (!pipelineState->IsValid())
	{
		printf("パイプラインステートの生成に失敗\n");
		return;
	}

	// マップに保存
	rootSignatureMap[name] = rootSignature;
	pipelineStateMap[name] = pipelineState;
}

std::shared_ptr<RootSignature> PipelineStateManager::get_root_signature(const std::string& name)
{
	if (rootSignatureMap.count(name)) {
		return rootSignatureMap[name];
	}
	return nullptr;
}

std::shared_ptr<PipelineState> PipelineStateManager::get_pipeline_state(const std::string& name)
{
	if (pipelineStateMap.count(name)) {
		return pipelineStateMap[name];
	}
	return nullptr;
}
