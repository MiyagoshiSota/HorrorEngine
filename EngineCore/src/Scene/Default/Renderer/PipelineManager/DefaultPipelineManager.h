#pragma once
#include "Renderer/Pass/PostProcess/Manager/PostProcessManager.h"
#include "Renderer/Pass/PostProcess/Pass/FXAAPass.h"
#include "Renderer/Pass/PostProcess/Pass/TAAPass.h"
#include "Renderer/Pass/PostProcess/Pass/UnjitterPass.h"
#include "Renderer/Pass/ShadowProcess/Pass/SimpleShadowMapPass.h"
#include "Renderer/Pass/ShadowProcess/Pass/RayTracedShadowPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SkyboxPass.h"
#include "Renderer/Pass/RenderProcess/Pass/DebugPass.h"
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

/// アンチエイリアシング設定
struct AASettings
{
	static constexpr UINT kMSAASampleCount = 8;
	bool msaaEnabled = false;  
	bool fxaaEnabled = false;
	bool taaEnabled = false;
};

class DefaultPipelineManager : public IPipelineManager
{
public:
    DefaultPipelineManager();

    void Execute() override;

    /// フレーム内で実行するパスのカテゴリ別リスト
    struct FramePassList
    {
        // Shadow Phase (ShadowMap, RayTracedShadow)
        std::vector<std::shared_ptr<IRenderPass>> shadows;
        // Geometry Phase (GBuffer, Forward Opaque)
        std::vector<std::shared_ptr<IRenderPass>> geometries;
        // Lighting Phase (Deferred Only: AO, GI, Reflection, LightingComposite, SSR)
        std::vector<std::shared_ptr<IRenderPass>> lightings;
        // Transparent Phase (Skybox, Particles)
        std::vector<std::shared_ptr<IRenderPass>> transparents;
        // PostProcess Phase (TAA, Bloom, ToneMap, FXAA, UI)
        std::vector<std::shared_ptr<IRenderPass>> postProcesses;
        // Debug Phase
        std::vector<std::shared_ptr<IRenderPass>> debugs;

        void Clear()
        {
            shadows.clear();
            geometries.clear();
            lightings.clear();
            transparents.clear();
            postProcesses.clear();
            debugs.clear();
        }
    };

public:
	// 各種PassのGetter
	std::shared_ptr<PostProcessManager> GetPostProcessManager() { return m_postProcessManager; };
    std::shared_ptr<SkyboxPass> GetSkyboxPass() { return m_skyboxPass; };
	std::shared_ptr<FXAAPass> GetFXAAPass() { return m_fxaaPass; }
	std::shared_ptr<TAAPass> GetTAAPass() { return m_taaPass; }
	std::shared_ptr<UnjitterPass> GetUnjitterPass() { return m_unjitterPass; }

	// AASettingsのGetter
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

	// RTGIデノイズモード
	void SetRTGIDenoiseMode(RTAODenoiseMode mode);
	RTAODenoiseMode GetRTGIDenoiseMode() const;

	// デファード/フォワードレンダリングの切り替え
	void SetDeferredRendering(bool useDeferred) { m_useDeferred = useDeferred; }
	bool IsDeferredRendering() const { return m_useDeferred; }

	// SSAOの有効/無効を切り替え
	void SetSSAOEnabled(bool enabled) { m_ssaoEnabled = enabled; }
	bool IsSSAOEnabled() const { return m_ssaoEnabled; }

	// SSRの有効/無効を切り替え
	void SetSSREnabled(bool enabled) { m_ssrEnabled = enabled; }
	bool IsSSREnabled() const { return m_ssrEnabled; }

	// DebugPassの有効/無効を切り替え
	void SetDebugPassEnabled(bool enabled) { m_debugPassEnabled = enabled; }
	bool IsDebugPassEnabled() const { return m_debugPassEnabled; }

	// 各パスへのアクセス
	std::shared_ptr<SSAOPass> GetSSAOPass() { return m_ssaoPass; }
	std::shared_ptr<SSRPass> GetSSRPass() { return m_ssrPass; }
	std::shared_ptr<RTAOPass> GetRTAOPass() { return m_rayTracedAOPass; }
	std::shared_ptr<RTAODenoisePass> GetRTAODenoisePass() { return m_rtaoDenoisePass; }
	std::shared_ptr<RTGIPass> GetRayTracedGIPass() { return m_rayTracedGIPass; }
	std::shared_ptr<RTGIDenoisePass> GetRTGIDenoisePass() { return m_rtgiDenoisePass; }

	/// テクスチャプレビュー用。ConstRenderPrefの名前でバッファを取得
	std::shared_ptr<ITargetBase> GetRenderTargetForPreview(const char* name, class DefaultScene* scene = nullptr) const;
	std::shared_ptr<RTReflectionPass> GetRayTracedReflectionPass() { return m_rayTracedReflectionPass; }

private:
    // フレームごとの実行パスリストを構築する
    void SetupFrame(RenderContext& context);

    // RenderContext にレンダーターゲットを一括登録するヘルパー
    void SetupRenderTargetsToContext(RenderContext& context);

    // 複雑なポストプロセスチェーンを実行するヘルパー
    void ExecutePostProcessChain(RenderContext& context);

private:
    FramePassList m_framePasses;

	std::shared_ptr<RenderTarget> m_sceneColor; // シーンカラー
	std::shared_ptr<RenderTarget> m_msaaTarget; // MSAAターゲット
	std::shared_ptr<DepthStencilTarget> m_shadowDepth; // シャドウ深度
	std::shared_ptr<DepthStencilTarget> m_msaaDepth; // MSAA深度
	std::shared_ptr<DepthStencilTarget> m_sceneDepth; // シーン深度
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
	bool m_debugPassEnabled = true;

	std::shared_ptr<SimpleShadowMapPass> m_simpleShadowMapPass; // シンプルシャドウマップパス
	std::shared_ptr<RayTracedShadowPass> m_rayTracedShadowPass; // レイトレースシャドウパス
	std::shared_ptr<RTAOPass> m_rayTracedAOPass; // レイトレースAOパス
	std::shared_ptr<RTGIPass> m_rayTracedGIPass; // レイトレースGIパス
	std::shared_ptr<RTGIDenoisePass> m_rtgiDenoisePass; // レイトレースGIデノイズパス
	std::shared_ptr<RTReflectionPass> m_rayTracedReflectionPass; // レイトレース反射パス
	std::shared_ptr<PostProcessManager> m_postProcessManager; // ポストプロセスマネージャ
	std::shared_ptr<FXAAPass> m_fxaaPass; // FXAAパス
	std::shared_ptr<TAAPass> m_taaPass; // TAAパス
	std::shared_ptr<UnjitterPass> m_unjitterPass; // Unjitterパス
	std::shared_ptr<TempRenderTargetPool> m_tempRenderTargetPool; // 一時的なRenderTargetプール
    std::shared_ptr<SkyboxPass> m_skyboxPass; // スカイボックスパス
	std::shared_ptr<DebugPass> m_debugPass; // デバッグパス
	std::shared_ptr<LightingPass> m_lightingPass; // ライティングパス
	std::shared_ptr<SSAOPass> m_ssaoPass; // SSAOパス
	std::shared_ptr<RTAODenoisePass> m_rtaoDenoisePass; // RTAOデノイズパス
	std::shared_ptr<SSRPass> m_ssrPass; // SSRパス
	std::shared_ptr<SSRCompositePass> m_ssrCompositePass; // SSRコンポジットパス
};
