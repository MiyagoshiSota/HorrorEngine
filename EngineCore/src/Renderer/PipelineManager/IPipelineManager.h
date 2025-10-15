#pragma once

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/Pass/RenderProcess/SceneRenderPassBase.h"
#include "Renderer/RenderContext/RenderContext.h"

class IPipelineManager
{
public:
	IPipelineManager() = default;
	virtual ~IPipelineManager() = default;
	virtual void Execute() = 0;
	void AddRenderProcessPass(const std::shared_ptr<SceneRenderPassBase>& renderPass){ m_sceneRenderPasses.push_back(renderPass); };
	
protected:
	std::vector<std::shared_ptr<SceneRenderPassBase>> m_sceneRenderPasses;
};

