#pragma once
#include "Renderer/Pass/ComputeProcess/RainParticleSystem.h"
#include "Renderer/Pass/PostProcess/Manager/PostProcessManager.h"
#include "Renderer/Pass/PostProcess/Pass/FXAAPass.h"
#include "Renderer/Pass/PostProcess/Pass/TAAPass.h"
#include "Renderer/Pass/ShadowProcess/Pass/CascadedShadowMapPass.h"
#include "Renderer/Pass/ShadowProcess/Pass/SimpleShadowMapPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SkyboxPass.h"
#include "Renderer/PipelineManager/IPipelineManager.h"
#include "Renderer/Target/RenderTarget.h"
#include "Renderer/Target/DepthStencilTarget.h"

/// アンチエイリアシング設定（UIで変更可能）
struct AASettings
{
	static constexpr UINT kMSAASampleCount = 8; // 固定値
	bool msaaEnabled = true;  // MSAA ON/OFF
	bool fxaaEnabled = false;
	bool taaEnabled = false;
};

class DefaultPipelineManager : public IPipelineManager
{
public:
    DefaultPipelineManager();
    
    void Execute() override;

public:
	std::shared_ptr<PostProcessManager> GetPostProcessManager() { return m_postProcessManager; };
    std::shared_ptr<SkyboxPass> GetSkyboxPass() { return m_skyboxPass; };
	std::shared_ptr<FXAAPass> GetFXAAPass() { return m_fxaaPass; }
	std::shared_ptr<TAAPass> GetTAAPass() { return m_taaPass; }

	AASettings& GetAASettings() { return m_aaSettings; }
	const AASettings& GetAASettings() const { return m_aaSettings; }
	void SetMSAAEnabled(bool enabled) { m_aaSettings.msaaEnabled = enabled; }
	void SetFXAAEnabled(bool enabled) { m_aaSettings.fxaaEnabled = enabled; }
	void SetTAAEnabled(bool enabled) { m_aaSettings.taaEnabled = enabled; }

private:
    std::shared_ptr<RenderTarget> m_sceneColor;
	std::shared_ptr<RenderTarget> m_msaaTarget;
	std::shared_ptr<DepthStencilTarget> m_shadowDepth;
	std::shared_ptr<DepthStencilTarget> m_cascadedShadowDepth;
	std::shared_ptr<DepthStencilTarget> m_msaaDepth;
	std::shared_ptr<DepthStencilTarget> m_sceneDepth;
	std::shared_ptr<RenderTarget> m_tmpColorA;
    std::shared_ptr<RenderTarget> m_tmpColorB;
	std::shared_ptr<RenderTarget> m_historyBuffer; // TAA用履歴バッファ

private:
	AASettings m_aaSettings;

	std::shared_ptr<SimpleShadowMapPass> m_simpleShadowMapPass;
	//std::shared_ptr<CascadesShadowMapPass> m_simpleShadowMapPass;
    std::shared_ptr<PostProcessManager> m_postProcessManager;
	std::shared_ptr<FXAAPass> m_fxaaPass;
	std::shared_ptr<TAAPass> m_taaPass;
    std::shared_ptr<RainParticleSystem> m_rainParticleSystem;
	std::shared_ptr<TempRenderTargetPool> m_tempRenderTargetPool;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
};

