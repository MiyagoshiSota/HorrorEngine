#pragma once
#include <memory>
#include <string>

#include "Renderer/PipelineManager/PipelineStateManager.h"

class PSOLoader
{
public:
	static std::shared_ptr<PipelineStateManager> load_from_file(const std::string& filePath, std::shared_ptr<PipelineStateManager> manager);
};

