#pragma once

#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/RenderPass/IRenderPass.h"

class IPipelineManager
{
public:
	IPipelineManager() = default;
	virtual ~IPipelineManager() = default;
	virtual void Execute() = 0;
	void AddRenderPass(const std::shared_ptr<IRenderPass>& renderPass){ m_renderPasses.push_back(renderPass); };
	
protected:
	std::vector<std::shared_ptr<IRenderPass>> m_renderPasses;
};

