#pragma once
#include "Renderer/Pass/ComputeProcess/RainParticleSystem.h"
#include "Renderer/Pass/PostProcess/Manager/PostProcessManager.h"
#include "Renderer/PipelineManager/IPipelineManager.h"
#include "Renderer/Target/RenderTarget.h"
#include "Renderer/Target/DepthStencilTarget.h"

class DefaultPipelineManager : public IPipelineManager
{
public:
    DefaultPipelineManager();
    
    void Execute() override;

public:
    std::shared_ptr<PostProcessManager> get_post_process_manager() { return m_postProcessManager; };

private:
    std::shared_ptr<RenderTarget> m_sceneColor;
	std::shared_ptr<RenderTarget> m_msaaTarget;
	std::shared_ptr<DepthStencilTarget> m_msaaDepth;
	std::shared_ptr<DepthStencilTarget> m_sceneDepth;
	std::shared_ptr<RenderTarget> m_tmpColorA;
    std::shared_ptr<RenderTarget> m_tmpColorB;

private:
    std::shared_ptr<PostProcessManager> m_postProcessManager;
    std::shared_ptr<RainParticleSystem> m_rainParticleSystem;
	std::shared_ptr<TempRenderTargetPool> m_tempRenderTargetPool;
};

