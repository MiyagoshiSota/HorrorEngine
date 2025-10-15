#pragma once
#include "Renderer/Pass/PostProcess/Manager/PostProcessManager.h"
#include "Renderer/PipelineManager/IPipelineManager.h"
#include "Renderer/Target/RenderTarget.h"
#include "Renderer/Target/DepthStencilTarget.h"

class DefaultPipelineManager : public IPipelineManager
{
public:
    DefaultPipelineManager();
    
    void Execute() override;

private:
    std::shared_ptr<RenderTarget> m_sceneColor;
	std::shared_ptr<DepthStencilTarget> m_sceneDepth;
	std::shared_ptr<RenderTarget> m_tmpColorA;
    std::shared_ptr<RenderTarget> m_tmpColorB;
    std::shared_ptr<PostProcessManager> m_postProcessManager;
};

