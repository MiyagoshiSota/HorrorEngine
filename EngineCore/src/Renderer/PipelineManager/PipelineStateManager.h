#pragma once
#include <d3d12.h>
#include <map>

#include "Renderer/Graphics/PipelineState.h"
#include "Renderer/Graphics/RootSignature.h"

class PipelineStateManager
{
public:
	void new_create(std::wstring vsfilePath, std::wstring psfilePath, bool isInputLayout, bool isDepthFormat, const std::shared_ptr<
	                RootSignatureBuilder>& builder, const std::string& name);

	// 各種getter
	std::shared_ptr<RootSignature> get_root_signature(const std::string& name);
	std::shared_ptr<PipelineState> get_pipeline_state(const std::string& name);

private:
	std::map<std::string, std::shared_ptr<RootSignature>> rootSignatureMap;
	std::map < std::string, std::shared_ptr<PipelineState>> pipelineStateMap;
};

