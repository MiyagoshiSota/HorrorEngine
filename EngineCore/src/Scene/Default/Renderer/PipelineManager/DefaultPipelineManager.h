#pragma once
#include "Renderer/Pass/ComputeProcess/RainParticleSystem.h"
#include "Renderer/Pass/PostProcess/Manager/PostProcessManager.h"
#include "Renderer/Pass/PostProcess/Pass/FXAAPass.h"
#include "Renderer/Pass/PostProcess/Pass/TAAPass.h"
#include "Renderer/Pass/PostProcess/Pass/UnjitterPass.h"
#include "Renderer/Pass/ShadowProcess/Pass/CascadedShadowMapPass.h"
#include "Renderer/Pass/ShadowProcess/Pass/SimpleShadowMapPass.h"
#include "Renderer/Pass/ShadowProcess/Pass/RayTracedShadowPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SkyboxPass.h"
#include "Renderer/Pass/RenderProcess/Pass/LightingPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SSAOPass.h"
#include "Renderer/Pass/RenderProcess/Pass/RTAODenoisePass.h"
#include "Renderer/Pass/RenderProcess/Pass/RTAOPass.h"
#include "Renderer/Pass/RenderProcess/Pass/RTGIPass.h"
#include "Renderer/Pass/RenderProcess/Pass/RTGIDenoisePass.h"
#include "Renderer/Pass/RenderProcess/Pass/RTReflectionPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SSRPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SSRCompositePass.h"
#include "Renderer/RenderContext/ShadowTypes.h"
#include "Renderer/PipelineManager/IPipelineManager.h"
#include "Renderer/Target/ITargetBase.h"
#include "Renderer/Target/RenderTarget.h"
#include "Renderer/Target/DepthStencilTarget.h"

/// アンチエイリアシング設定（UIで変更可能）
struct AASettings
{
	static constexpr UINT kMSAASampleCount = 8; // 固定値
	bool msaaEnabled = false;  // MSAA ON/OFF (Forward only)
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
	std::shared_ptr<UnjitterPass> GetUnjitterPass() { return m_unjitterPass; }

	AASettings& GetAASettings() { return m_aaSettings; }
	const AASettings& GetAASettings() const { return m_aaSettings; }
	void SetMSAAEnabled(bool enabled) { m_aaSettings.msaaEnabled = enabled; }
	void SetFXAAEnabled(bool enabled) { m_aaSettings.fxaaEnabled = enabled; }
	void SetTAAEnabled(bool enabled) { m_aaSettings.taaEnabled = enabled; }

	// Ray Traced Shadowの有効/無効を切り替え
	void SetRayTracedShadowEnabled(bool enabled);
	bool IsRayTracedShadowEnabled() const;

	// RTAOの有効/無効を切り替え
	void SetRayTracedAOEnabled(bool enabled);
	bool IsRayTracedAOEnabled() const;

	// RTGIの有効/無効を切り替え
	void SetRayTracedGIEnabled(bool enabled);
	bool IsRayTracedGIEnabled() const;

	// RT Reflectionの有効/無効を切り替え
	void SetRayTracedReflectionEnabled(bool enabled);
	bool IsRayTracedReflectionEnabled() const;

	// RTAOデノイズモード: Off=コピーのみ, Bilateral=5x5 Bilateral, Separable=Separable Bilateral
	void SetRTAODenoiseMode(RTAODenoiseMode mode);
	RTAODenoiseMode GetRTAODenoiseMode() const;

	// RTGIデノイズモード（AOと同じアルゴリズム、RGB対応）
	void SetRTGIDenoiseMode(RTAODenoiseMode mode);
	RTAODenoiseMode GetRTGIDenoiseMode() const;

	// デファード / フォワードレンダリングの切り替え
	void SetDeferredRendering(bool useDeferred) { m_useDeferred = useDeferred; }
	bool IsDeferredRendering() const { return m_useDeferred; }

	// SSAOの有効/無効を切り替え
	void SetSSAOEnabled(bool enabled) { m_ssaoEnabled = enabled; }
	bool IsSSAOEnabled() const { return m_ssaoEnabled; }

	// SSRの有効/無効を切り替え
	void SetSSREnabled(bool enabled) { m_ssrEnabled = enabled; }
	bool IsSSREnabled() const { return m_ssrEnabled; }

	// 各パスへのアクセス（ランタイムパラメータ変更用）
	std::shared_ptr<SSAOPass> GetSSAOPass() { return m_ssaoPass; }
	std::shared_ptr<SSRPass> GetSSRPass() { return m_ssrPass; }
	std::shared_ptr<RTAOPass> GetRTAOPass() { return m_rayTracedAOPass; }
	std::shared_ptr<RTAODenoisePass> GetRTAODenoisePass() { return m_rtaoDenoisePass; }
	std::shared_ptr<RTGIPass> GetRayTracedGIPass() { return m_rayTracedGIPass; }
	std::shared_ptr<RTGIDenoisePass> GetRTGIDenoisePass() { return m_rtgiDenoisePass; }

	/// テクスチャプレビュー用。ConstRenderPrefの名前でバッファを取得。nullptrの場合は非アクティブ。
	/// sceneを渡すとRTGIBuffer/RTAORaw等のScene所有バッファも解決可能。
	std::shared_ptr<ITargetBase> GetRenderTargetForPreview(const char* name, class DefaultScene* scene = nullptr) const;
	std::shared_ptr<RTReflectionPass> GetRayTracedReflectionPass() { return m_rayTracedReflectionPass; }

private:
    // ポストプロセス全体（FXAA/TAAの有無もここで分岐）
    void ExecutePostProcess(RenderContext& context);

    // TAAの実行と履歴バッファ更新
    void ApplyTAA(RenderContext& context);

    // FXAAのみを適用する場合の処理（中間バッファ → バックバッファ）
    void ApplyFXAAAfterPostProcess(RenderContext& context, std::shared_ptr<ITargetBase> sourceRT);

private:
    std::shared_ptr<RenderTarget> m_sceneColor;
	std::shared_ptr<RenderTarget> m_msaaTarget;
	std::shared_ptr<DepthStencilTarget> m_shadowDepth;
	std::shared_ptr<DepthStencilTarget> m_cascadedShadowDepth;
	std::shared_ptr<DepthStencilTarget> m_msaaDepth;
	std::shared_ptr<DepthStencilTarget> m_sceneDepth;
	std::shared_ptr<RenderTarget> m_tmpColorA;
    std::shared_ptr<RenderTarget> m_tmpColorB;
	std::shared_ptr<RenderTarget> m_historyBuffer; // 履歴バッファ
	std::shared_ptr<RenderTarget> m_motionVectorBuffer; // モーションベクターバッファ（RG16F、MSAA対応）
	std::shared_ptr<RenderTarget> m_motionVectorResolved; // モーションベクターバッファ（Resolve後）
	std::shared_ptr<RenderTarget> m_normalBuffer;       // レイトレ用：法線（MSAA時は8サンプル）
	std::shared_ptr<RenderTarget> m_worldPositionBuffer; // レイトレ用：ワールド位置（MSAA時は8サンプル）
	std::shared_ptr<RenderTarget> m_normalBufferNonMSAA;       // G-Buffer：法線（1x）
	std::shared_ptr<RenderTarget> m_worldPositionBufferNonMSAA; // G-Buffer：ワールド位置（1x）
	std::shared_ptr<RenderTarget> m_gbufferAlbedo;      // G-Buffer：アルベド（1x）
	std::shared_ptr<RenderTarget> m_gbufferMaterial;   // G-Buffer：roughness, metallic, AO, emissive.r
	std::shared_ptr<RenderTarget> m_gbufferEmissive;  // G-Buffer：emissive.g, emissive.b
	std::shared_ptr<RenderTarget> m_ssaoBuffer;      // SSAO出力（R8）
	std::shared_ptr<RenderTarget> m_rtaoDenoiseBuffer; // RTAOデノイズ出力（R32）
	std::shared_ptr<RenderTarget> m_rtaoDenoiseTemp;   // Separable用中間バッファ（R32）
	std::shared_ptr<RenderTarget> m_ssrBuffer;        // SSR反射カラー出力（RGBA）
	std::shared_ptr<RenderTarget> m_rtgiDenoisedBuffer; // RTGIデノイズ出力（RGBA）
	std::shared_ptr<RenderTarget> m_rtgiDenoiseTemp;   // RTGIデノイズ中間バッファ（RGBA）

private:
	bool m_useDeferred = true;  // true=デファード（G-Buffer+LightingPass）, false=フォワード（SimplePS 1パス）
	AASettings m_aaSettings;
	bool m_ssaoEnabled = false;
	bool m_ssrEnabled = false;

	std::shared_ptr<SimpleShadowMapPass> m_simpleShadowMapPass;
	std::shared_ptr<RayTracedShadowPass> m_rayTracedShadowPass;
	std::shared_ptr<RTAOPass> m_rayTracedAOPass;
	std::shared_ptr<RTGIPass> m_rayTracedGIPass;
	std::shared_ptr<RTGIDenoisePass> m_rtgiDenoisePass;
	std::shared_ptr<RTReflectionPass> m_rayTracedReflectionPass;
	//std::shared_ptr<CascadesShadowMapPass> m_simpleShadowMapPass;
	std::shared_ptr<PostProcessManager> m_postProcessManager;
	std::shared_ptr<FXAAPass> m_fxaaPass;
	std::shared_ptr<TAAPass> m_taaPass;
	std::shared_ptr<UnjitterPass> m_unjitterPass;
    std::shared_ptr<RainParticleSystem> m_rainParticleSystem;
	std::shared_ptr<TempRenderTargetPool> m_tempRenderTargetPool;
    std::shared_ptr<SkyboxPass> m_skyboxPass;
	std::shared_ptr<LightingPass> m_lightingPass;
	std::shared_ptr<SSAOPass> m_ssaoPass;
	std::shared_ptr<RTAODenoisePass> m_rtaoDenoisePass;
	std::shared_ptr<SSRPass> m_ssrPass;
	std::shared_ptr<SSRCompositePass> m_ssrCompositePass;
};

