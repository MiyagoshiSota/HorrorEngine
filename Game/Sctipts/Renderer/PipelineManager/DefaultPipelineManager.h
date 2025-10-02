#pragma once
#include "Renderer/PipelineManager/IPipelineManager.h"
#include "Renderer/RenderPass/GeometryPass.h"
#include "Renderer/RenderPass/SceneSetupPass.h"
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
};

