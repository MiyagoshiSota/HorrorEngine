#include "DefaultPipelineManager.h"

#include "Core/App.h"
#include "Modules/PublicConst/ConstPathPref.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Modules/Renderer/RendereUtility.h"
#include "Renderer/Engine.h"
#include "Renderer/Pass/RenderProcess/Pass/DebugPass.h"
#include "Renderer/Pass/RenderProcess/Pass/GeometryPass.h"
#include "Renderer/Pass/RenderProcess/Pass/LightingPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SSAOPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SSRPass.h"
#include "Renderer/Pass/RenderProcess/Pass/SSRCompositePass.h"
#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"
#include "Scene/Skybox/SkyboxManager.h"
#include "Scene/RayTracing/RayTracedShadowManager.h"
#include "Scene/RayTracing/RayTracedGIManager.h"
#include "Scene/RayTracing/RayTracedReflectionManager.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Renderer/Target/DepthStencilTarget.h"
#include "Renderer/RenderContext/ShadowTypes.h"
#include <d3dx12.h>
#include <DirectXMath.h>
#include <cstring>

DefaultPipelineManager::DefaultPipelineManager()
{
	
	m_simpleShadowMapPass = std::make_shared<SimpleShadowMapPass>();
	m_skyboxPass = std::make_shared<SkyboxPass>();
	AddRenderProcessPass(std::make_shared<GeometryPass>());
	m_debugPass = std::make_shared<DebugPass>();

	if (g_Engine->IsDxrSupported())
	{
		m_rayTracedShadowPass = std::make_shared<RayTracedShadowPass>();
		m_rayTracedShadowPass->SetEnabled(false);
		m_rayTracedAOPass = std::make_shared<RTAOPass>();
		m_rayTracedAOPass->SetEnabled(false);
		m_rayTracedGIPass = std::make_shared<RTGIPass>();
		m_rayTracedGIPass->SetEnabled(false);
		m_rayTracedReflectionPass = std::make_shared<RTReflectionPass>();
		m_rayTracedReflectionPass->SetEnabled(false);
	}

	// メインカラー・深度・TAA履歴
	m_sceneColor = std::make_shared<RenderTarget>();
	m_sceneColor->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_historyBuffer = std::make_shared<RenderTarget>();
	m_historyBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	m_shadowDepth = std::make_shared<DepthStencilTarget>();
	m_shadowDepth->Create(g_Engine->Device(), 2048, 2048, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_sceneDepth = std::make_shared<DepthStencilTarget>();
	m_sceneDepth->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	// MSAA用
	m_msaaTarget = std::make_shared<RenderTarget>();
	m_msaaTarget->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_msaaDepth = std::make_shared<DepthStencilTarget>();
	m_msaaDepth->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	// モーションベクターバッファ（TAA用 MSAA版/非MSAA版）
	m_motionVectorBuffer = std::make_shared<RenderTarget>();
	m_motionVectorBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16_FLOAT, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_motionVectorResolved = std::make_shared<RenderTarget>();
	m_motionVectorResolved->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	// G-Buffer
	m_normalBufferNonMSAA = std::make_shared<RenderTarget>();
	m_normalBufferNonMSAA->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_worldPositionBufferNonMSAA = std::make_shared<RenderTarget>();
	m_worldPositionBufferNonMSAA->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_gbufferAlbedo = std::make_shared<RenderTarget>();
	m_gbufferAlbedo->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_gbufferMaterial = std::make_shared<RenderTarget>();
	m_gbufferMaterial->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_gbufferEmissive = std::make_shared<RenderTarget>();
	m_gbufferEmissive->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	// フォワード用MRT（法線・ワールド位置）
	m_normalBuffer = std::make_shared<RenderTarget>();
	m_normalBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_worldPositionBuffer = std::make_shared<RenderTarget>();
	m_worldPositionBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	// AO（SSAO / RTAO + デノイズ）
	m_ssaoBuffer = std::make_shared<RenderTarget>();
	m_ssaoBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtaoDenoiseBuffer = std::make_shared<RenderTarget>();
	m_rtaoDenoiseBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtaoDenoiseTemp = std::make_shared<RenderTarget>();
	m_rtaoDenoiseTemp->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	// GI・反射・SSR
	m_ssrBuffer = std::make_shared<RenderTarget>();
	m_ssrBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtgiDenoisedBuffer = std::make_shared<RenderTarget>();
	m_rtgiDenoisedBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtgiDenoiseTemp = std::make_shared<RenderTarget>();
	m_rtgiDenoiseTemp->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));

	// ポストプロセス
	m_postProcessManager = std::make_shared<PostProcessManager>();
	m_postProcessManager->LoadPresets(ConstPathPref::kPostProcessPresetsPath);
	m_postProcessManager->Init();
	m_fxaaPass = std::make_shared<FXAAPass>();
	m_taaPass = std::make_shared<TAAPass>();
	m_unjitterPass = std::make_shared<UnjitterPass>();

	// ライティング・デノイズパス
	m_tempRenderTargetPool = std::make_shared<TempRenderTargetPool>();
	m_lightingPass = std::make_shared<LightingPass>();
	m_ssaoPass = std::make_shared<SSAOPass>();
	m_rtaoDenoisePass = std::make_shared<RTAODenoisePass>();
	m_rtgiDenoisePass = std::make_shared<RTGIDenoisePass>();
	m_ssrPass = std::make_shared<SSRPass>();
	m_ssrCompositePass = std::make_shared<SSRCompositePass>();
}

void DefaultPipelineManager::SetupFrame(RenderContext& context)
{
	m_framePasses.Clear();

	// Shadow Phase
	// Ray Traced Shadow が有効ならそちらを、無ければ SimpleShadowMap を登録
	if (m_rayTracedShadowPass && m_rayTracedShadowPass->IsEnabled())
	{
		m_framePasses.shadows.push_back(m_rayTracedShadowPass);
	}
	else if (m_simpleShadowMapPass)
	{
		m_framePasses.shadows.push_back(m_simpleShadowMapPass);
	}

	// Geometry Phase (G-Buffer / Forward Opaque)
	for (auto& pass : m_sceneRenderPasses)
	{
		if (pass)
		{
			m_framePasses.geometries.push_back(pass);
		}
	}

	// Lighting Phase
	if (m_useDeferred)
	{
		// AO (SSAO or RTAO + Denoise)
		if (m_rayTracedAOPass && m_rayTracedAOPass->IsEnabled())
		{
			m_framePasses.lightings.push_back(m_rayTracedAOPass);
			if (m_rtaoDenoisePass)
			{
				m_framePasses.lightings.push_back(m_rtaoDenoisePass);
			}
		}
		else if (m_ssaoPass)
		{
			m_ssaoPass->SetEnabled(m_ssaoEnabled);
			m_framePasses.lightings.push_back(m_ssaoPass);
		}

		// GI (RTGI + Denoise)
		if (m_rayTracedGIPass && m_rayTracedGIPass->IsEnabled())
		{
			m_framePasses.lightings.push_back(m_rayTracedGIPass);
			if (m_rtgiDenoisePass && GetRTGIDenoiseMode() != RTAODenoiseMode::Off)
			{
				m_framePasses.lightings.push_back(m_rtgiDenoisePass);
			}
		}

		// Reflection (RTReflection)
		if (m_rayTracedReflectionPass && m_rayTracedReflectionPass->IsEnabled())
		{
			m_framePasses.lightings.push_back(m_rayTracedReflectionPass);
		}

		// LightingPass (Main)
		if (m_lightingPass)
		{
			context.SetRTGIEnabled(m_rayTracedGIPass && m_rayTracedGIPass->IsEnabled());
			context.SetRTReflectionEnabled(m_rayTracedReflectionPass && m_rayTracedReflectionPass->IsEnabled());
			m_framePasses.lightings.push_back(m_lightingPass);
		}

		// SSRPass -> SSRCompositePass
		if (m_ssrEnabled && m_ssrPass && m_ssrCompositePass)
		{
			m_framePasses.lightings.push_back(m_ssrPass);
			m_framePasses.lightings.push_back(m_ssrCompositePass);
		}
	}

	// Transparent Phase (Skybox, Particles)
	if (m_skyboxPass)
		m_framePasses.transparents.push_back(m_skyboxPass);

	// PostProcess Phase
	// NOTE: PostProcessManager::GetActivePasses() -> TAA (有効なら) -> FXAA (有効なら)
	for (const auto& pass : m_postProcessManager->GetActivePasses())
	{
		if (pass)
		{
			m_framePasses.postProcesses.push_back(pass);
		}
	}
	if (m_aaSettings.taaEnabled && m_taaPass)
	{
		m_framePasses.postProcesses.push_back(m_taaPass);
	}
	if (m_aaSettings.fxaaEnabled && m_fxaaPass)
	{
		m_framePasses.postProcesses.push_back(m_fxaaPass);
	}

	// Debug Phase
	if (m_debugPass && m_debugPassEnabled)
	{
		m_framePasses.debugs.push_back(m_debugPass);
	}
}

void DefaultPipelineManager::SetupRenderTargetsToContext(RenderContext& context)
{
	// シーンの取得
	auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);

	// メインカラー・深度・TAA履歴
	context.AddRenderTarget(ConstRenderPref::SceneColor, m_sceneColor);
	context.AddRenderTarget(ConstRenderPref::SceneDepth, m_sceneDepth);
	context.AddRenderTarget(ConstRenderPref::HistoryBuffer, m_historyBuffer);
	context.AddRenderTarget(ConstRenderPref::MSAART, m_msaaTarget);
	context.AddRenderTarget(ConstRenderPref::MSAA_Depth, m_msaaDepth);

	// デファード用
	if (m_useDeferred)
	{
		// G-Buffer
		context.AddRenderTarget(ConstRenderPref::GBufferAlbedo, m_gbufferAlbedo);
		context.AddRenderTarget(ConstRenderPref::GBufferMaterial, m_gbufferMaterial);
		context.AddRenderTarget(ConstRenderPref::GBufferEmissive, m_gbufferEmissive);
		context.AddRenderTarget(ConstRenderPref::MotionVectorBuffer, m_motionVectorResolved);
		context.AddRenderTarget(ConstRenderPref::NormalBuffer, m_normalBufferNonMSAA);
		context.AddRenderTarget(ConstRenderPref::WorldPositionBuffer, m_worldPositionBufferNonMSAA);
		
		// AO (SSAO or RTAO + Denoise)
		if (m_rayTracedAOPass && m_rayTracedAOPass->IsEnabled() && defaultScene)
		{
			auto aoManager = defaultScene->GetRayTracedAOManager();
			if (aoManager && aoManager->IsValid())
			{
				auto aoData = aoManager->GetRenderData(g_Engine->CurrentBackBufferIndex(), defaultScene->GetRayTracedShadowManager()->GetASManager());
				if (aoData.aoTarget)
				{
					context.AddRenderTarget(ConstRenderPref::RTAORaw, aoData.aoTarget);
					context.AddRenderTarget(ConstRenderPref::SSAOBuffer, m_rtaoDenoiseBuffer);
					context.AddRenderTarget(ConstRenderPref::RTAODenoiseTemp, m_rtaoDenoiseTemp);
				}
				else
				{
					context.AddRenderTarget(ConstRenderPref::SSAOBuffer, m_ssaoBuffer);
				}
			}
			else
				context.AddRenderTarget(ConstRenderPref::SSAOBuffer, m_ssaoBuffer);
		}
		else
			context.AddRenderTarget(ConstRenderPref::SSAOBuffer, m_ssaoBuffer);

		// GI (RTGI + Denoise)
		if (m_rayTracedGIPass && m_rayTracedGIPass->IsEnabled() && defaultScene)
		{
			auto giManager = defaultScene->GetRayTracedGIManager();
			if (giManager && giManager->IsValid())
			{
				auto giData = giManager->GetRenderData(g_Engine->CurrentBackBufferIndex(), defaultScene->GetRayTracedShadowManager()->GetASManager());
				if (giData.giTarget)
				{
					const bool rtgiDenoiseEnabled = m_rtgiDenoisePass && GetRTGIDenoiseMode() != RTAODenoiseMode::Off;
					if (rtgiDenoiseEnabled)
					{
						context.AddRenderTarget(ConstRenderPref::RTGIRaw, giData.giTarget);
						context.AddRenderTarget(ConstRenderPref::RTGIBuffer, m_rtgiDenoisedBuffer);
						context.AddRenderTarget(ConstRenderPref::RTGIDenoiseTemp, m_rtgiDenoiseTemp);
					}
					else
						context.AddRenderTarget(ConstRenderPref::RTGIBuffer, giData.giTarget);
				}
			}
		}

		// Reflection (RTReflection)
		if (m_rayTracedReflectionPass && m_rayTracedReflectionPass->IsEnabled() && defaultScene)
		{
			auto reflectionManager = defaultScene->GetRayTracedReflectionManager();
			if (reflectionManager && reflectionManager->IsValid())
			{
				auto reflData = reflectionManager->GetRenderData(g_Engine->CurrentBackBufferIndex(), defaultScene->GetRayTracedShadowManager()->GetASManager());
				if (reflData.reflectionTarget)
					context.AddRenderTarget(ConstRenderPref::RTReflectionBuffer, reflData.reflectionTarget);
			}
		}

		// SSR (SSRBuffer)
		context.AddRenderTarget(ConstRenderPref::SSRBuffer, m_ssrBuffer);
	}
	else
	{
		// フォワード用
		if (m_aaSettings.msaaEnabled)
		{
			context.AddRenderTarget(ConstRenderPref::MotionVectorBuffer, m_motionVectorBuffer);
			context.AddRenderTarget(ConstRenderPref::NormalBuffer, m_normalBuffer);
			context.AddRenderTarget(ConstRenderPref::WorldPositionBuffer, m_worldPositionBuffer);
		}
		else
		{
			context.AddRenderTarget(ConstRenderPref::MotionVectorBuffer, m_motionVectorResolved);
			context.AddRenderTarget(ConstRenderPref::NormalBuffer, m_normalBufferNonMSAA);
			context.AddRenderTarget(ConstRenderPref::WorldPositionBuffer, m_worldPositionBufferNonMSAA);
		}
	}

	// Shadow
	if (defaultScene && m_rayTracedShadowPass && m_rayTracedShadowPass->IsEnabled())
	{
		// Ray Traced Shadow Managerの取得
		auto rayTracedShadowManager = defaultScene->GetRayTracedShadowManager();

		if (rayTracedShadowManager && rayTracedShadowManager->IsValid())
		{
			const UINT frameIndex = g_Engine->CurrentBackBufferIndex();
			auto renderData = rayTracedShadowManager->GetRenderData(frameIndex);
			if (renderData.shadowMapTarget)
			{
				context.AddRenderTarget(ConstRenderPref::ShadowMap, renderData.shadowMapTarget);
				context.SetUseRayTracedShadow(true);
				context.SetInvRayTracedShadowMapSize(1.0f / renderData.width, 1.0f / renderData.height);
			}
			else
			{
				context.AddRenderTarget(ConstRenderPref::ShadowMap, m_shadowDepth);
				context.SetUseRayTracedShadow(false);
			}
		}
		else
		{
			context.AddRenderTarget(ConstRenderPref::ShadowMap, m_shadowDepth);
			context.SetUseRayTracedShadow(false);
		}
	}
	else
	{
		context.AddRenderTarget(ConstRenderPref::ShadowMap, m_shadowDepth);
		context.SetUseRayTracedShadow(false);
	}

	if (defaultScene)
	{
		// Skybox
		auto skyboxManager = defaultScene->GetSkyboxManager();
		if (skyboxManager && skyboxManager->IsValid())
		{
			auto renderData = skyboxManager->GetRenderData();
			RenderContext::SkyboxData skyboxData;
			skyboxData.vertexBuffer = renderData.vertexBuffer;
			skyboxData.indexBuffer = renderData.indexBuffer;
			skyboxData.indexCount = renderData.indexCount;
			skyboxData.constantBuffer = renderData.constantBuffer;
			skyboxData.srvHandle = renderData.srvHandle;
			skyboxData.isValid = true;
			context.SetSkyboxData(skyboxData);
			context.SetSkyboxUpdateCallback([skyboxManager](DirectX::XMMATRIX viewProj) {
				skyboxManager->UpdateConstantBuffer(viewProj);
			});
		}

		// Ray Traced Shadow
		auto rayTracedShadowManager = defaultScene->GetRayTracedShadowManager();
		if (rayTracedShadowManager && rayTracedShadowManager->IsValid())
		{
			const UINT frameIndex = g_Engine->CurrentBackBufferIndex();
			auto renderData = rayTracedShadowManager->GetRenderData(frameIndex);
			context.SetRayTracedShadowData(renderData);
			context.SetRayTracedShadowUpdateCallback([rayTracedShadowManager](const RayTracedShadowSceneConstants& constants, UINT fi) {
				rayTracedShadowManager->UpdateSceneConstants(constants, fi);
			});
		}

		// Ray Traced AO
		auto aoManager = defaultScene->GetRayTracedAOManager();
		auto shadowManager = defaultScene->GetRayTracedShadowManager();
		if (aoManager && aoManager->IsValid() && shadowManager && shadowManager->IsValid())
		{
			const UINT frameIndex = g_Engine->CurrentBackBufferIndex();
			auto aoData = aoManager->GetRenderData(frameIndex, shadowManager->GetASManager());
			context.SetRayTracedAOData(aoData);
			context.SetRayTracedAOUpdateCallback([aoManager](const RayTracedAOConstants& constants, UINT fi) {
				aoManager->UpdateConstants(constants, fi);
			});
		}

		// Ray Traced GI
		auto giManager = defaultScene->GetRayTracedGIManager();
		if (giManager && giManager->IsValid() && shadowManager && shadowManager->IsValid())
		{
			const UINT frameIndex = g_Engine->CurrentBackBufferIndex();
			auto giData = giManager->GetRenderData(frameIndex, shadowManager->GetASManager());
			context.SetRayTracedGIData(giData);
			context.SetRayTracedGIUpdateCallback([giManager](const RayTracedGIConstants& constants, UINT fi) {
				giManager->UpdateConstants(constants, fi);
			});
		}

		// Ray Traced Reflection
		auto reflectionManager = defaultScene->GetRayTracedReflectionManager();
		if (reflectionManager && reflectionManager->IsValid() && shadowManager && shadowManager->IsValid())
		{
			const UINT frameIndex = g_Engine->CurrentBackBufferIndex();
			auto reflData = reflectionManager->GetRenderData(frameIndex, shadowManager->GetASManager());
			context.SetRayTracedReflectionData(reflData);
			context.SetRayTracedReflectionUpdateCallback([reflectionManager](const RayTracedReflectionConstants& constants, UINT fi) {
				reflectionManager->UpdateConstants(constants, fi);
			});
		}
	}
}

void DefaultPipelineManager::ExecutePostProcessChain(RenderContext& context)
{
	// PostProcessManagerの準備
	m_postProcessManager->PreparePassesForFrame();

	// デバッグキャプチャが有効なら、前フレームの結果をクリアしておく
	if (m_postProcessManager->IsCapturePassOutputsForDebug())
	{
		m_postProcessManager->ClearDebugPassOutputs();
	}

	// SceneColorのTransition
	auto sceneColorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
	if (sceneColorRT && sceneColorRT->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			sceneColorRT->GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		context.CommandList->ResourceBarrier(1, &barrier);
		sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// TmpColorAとTmpColorBの取得
	auto bufferA = context.GetRenderTarget(ConstRenderPref::TmpColorA);
	auto bufferB = context.GetRenderTarget(ConstRenderPref::TmpColorB);
	std::shared_ptr<ITargetBase> sourceRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
	const size_t numPasses = m_framePasses.postProcesses.size();
	// プリセット由来のパス名順序（PostProcessManager::GetActivePasses() と 1:1 対応）
	const auto& presetOrder = m_postProcessManager->GetCurrentPresetOrder();
	bool taaExecuted = false;

	// PostProcess Pass
	for (size_t i = 0; i < numPasses; ++i)
	{
		auto& pass = m_framePasses.postProcesses[i];
		const bool isLast = (i == numPasses - 1);
		std::shared_ptr<ITargetBase> destRT = (isLast && pass.get() != m_taaPass.get()) ? nullptr : ((i % 2 == 0) ? bufferA : bufferB);

		// SourceRTとDestRTの設定
		context.SetSourceRT(sourceRT);
		if (destRT)
			context.SetDestRT(destRT);

		// PostProcess Passの実行
		auto* ppPass = dynamic_cast<PostProcessPassBase*>(pass.get());
		if (isLast && !destRT && ppPass)
			ppPass->LastExecute(context, g_Engine->GetCurrentRtvHandle());
		else
			pass->Execute(context);

		// デバッグ用に出力を登録
		if (m_postProcessManager->IsCapturePassOutputsForDebug() && destRT && i < presetOrder.size())
		{
			m_postProcessManager->SetDebugPassOutput(presetOrder[i], destRT);
		}

		// TAA Passの実行
		if (pass.get() == m_taaPass.get())
			taaExecuted = true;

		// DestRTのTransition
		if (!isLast && destRT && destRT->GetCurrentState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		{
			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				destRT->GetResource(),
				destRT->GetCurrentState(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context.CommandList->ResourceBarrier(1, &barrier);
			destRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		// SourceRTの更新
		if (!isLast)
			sourceRT = destRT;
	}

	// TAA Passの実行
	if (taaExecuted)
	{
		// TmpColorAとTmpColorBの取得
		auto tmpColorA = std::dynamic_pointer_cast<RenderTarget>(context.GetRenderTarget(ConstRenderPref::TmpColorA));
		auto tmpColorB = std::dynamic_pointer_cast<RenderTarget>(context.GetRenderTarget(ConstRenderPref::TmpColorB));
		if (tmpColorA && tmpColorB)
		{
			// FXAAが無効なら
			if (!m_aaSettings.fxaaEnabled)
			{
				ID3D12Resource* backBuffer = g_Engine->GetCurrentBackBuffer();
				if (tmpColorB->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET && backBuffer)
				{
					D3D12_RESOURCE_BARRIER barriers[2] = {
						CD3DX12_RESOURCE_BARRIER::Transition(tmpColorB->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE),
						CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST)
					};
					context.CommandList->ResourceBarrier(2, barriers);
					tmpColorB->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
					context.CommandList->CopyResource(backBuffer, tmpColorB->GetResource());
					D3D12_RESOURCE_BARRIER barriersBack[2] = {
						CD3DX12_RESOURCE_BARRIER::Transition(tmpColorB->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
						CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET)
					};
					context.CommandList->ResourceBarrier(2, barriersBack);
					tmpColorB->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				}
			}

			// TmpColorBのTransition
			if (tmpColorB->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					tmpColorB->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				context.CommandList->ResourceBarrier(1, &barrier);
				tmpColorB->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}

			// TmpColorAのTransition
			if (tmpColorA->GetCurrentState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					tmpColorA->GetResource(),
					tmpColorA->GetCurrentState(),
					D3D12_RESOURCE_STATE_RENDER_TARGET);
				context.CommandList->ResourceBarrier(1, &barrier);
				tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
			}

			// UnjitterPassの実行
			m_unjitterPass->SetJitter(m_taaPass->GetCurrentJitter());
			context.SetSourceRT(tmpColorB);
			context.SetDestRT(tmpColorA);
			m_unjitterPass->Execute(context);

			// TmpColorAのTransition
			if (tmpColorA->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET)
			{
				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					tmpColorA->GetResource(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_COPY_SOURCE);
				context.CommandList->ResourceBarrier(1, &barrier);
				tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
			}

			// HistoryBufferのTransition
			if (m_historyBuffer->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			{
				D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m_historyBuffer->GetResource(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_COPY_DEST);
				context.CommandList->ResourceBarrier(1, &barrier);
				m_historyBuffer->SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
			}

			// HistoryBufferとTmpColorAのCopy
			context.CommandList->CopyResource(m_historyBuffer->GetResource(), tmpColorA->GetResource());

			// TmpColorAのTransition
			{
				D3D12_RESOURCE_BARRIER barriers[2] = {
					CD3DX12_RESOURCE_BARRIER::Transition(tmpColorA->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
					CD3DX12_RESOURCE_BARRIER::Transition(m_historyBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
				};
				context.CommandList->ResourceBarrier(2, barriers);
				tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				m_historyBuffer->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
		}
	}
}

void DefaultPipelineManager::Execute()
{
	// Jitterの更新
    if (m_aaSettings.taaEnabled && m_taaPass)
        m_taaPass->UpdateJitter();

    // RenderContextの構築
    RenderContext context(g_Engine->CommandList(), g_Scene->GetSceneCamera(), g_Scene->GetGameObjects(), m_sceneColor, m_sceneDepth, kWindowWidth, kWindowHeight, g_Scene->GetPipelineStateManager(), m_tempRenderTargetPool);

	// TAA Jitterの設定
    if (m_aaSettings.taaEnabled && m_taaPass)
        context.SetTAAJitter(m_taaPass->GetCurrentJitter(), true);

	// TmpColorAとTmpColorBの取得
	auto tmpColorA = m_tempRenderTargetPool->Get(static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight), DXGI_FORMAT_R8G8B8A8_UNORM);
	auto tmpColorB = m_tempRenderTargetPool->Get(static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight), DXGI_FORMAT_R8G8B8A8_UNORM);
	context.AddRenderTarget(ConstRenderPref::TmpColorA, tmpColorA);
	context.AddRenderTarget(ConstRenderPref::TmpColorB, tmpColorB);

	// RenderTargetsの設定
	SetupRenderTargetsToContext(context);

	// 実行順序の設定
	SetupFrame(context);

	// 実行フェーズ
	for (auto& pass : m_framePasses.shadows)
		pass->Execute(context);

    // ShadowContextを構築
    {
        ShadowContext sc;
        sc.mode = (m_rayTracedShadowPass && m_rayTracedShadowPass->IsEnabled())
            ? ShadowMode::RayTracedMask
            : ShadowMode::RasterDepth;
		
		// Shadowのシーンサイズとライトの距離
        const float kShadowSceneW = 50.0f;
        const float kShadowSceneH = 50.0f;
        const float kShadowNearZ = 0.0f;
        const float kShadowFarZ = 10.0f;
        const float kShadowLightDist = 25.0f;
        
		// ライトの方向ベクトルの計算
		auto lightManager = g_Scene->GetLightingManager();
        if (lightManager && !lightManager->GetDirectionalLights().empty())
        {
            DirectX::XMFLOAT3 lightDirF = lightManager->GetDirectionalLights()[0]->Direction;
            DirectX::XMVECTOR lightDir = DirectX::XMVector3Normalize(
                DirectX::XMVectorSet(lightDirF.x, lightDirF.y, lightDirF.z, 0.0f));
            DirectX::XMVECTOR targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
            DirectX::XMVECTOR lightPos = DirectX::XMVectorSubtract(
                targetPos, DirectX::XMVectorScale(lightDir, kShadowLightDist));
            DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtRH(
                lightPos, targetPos, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
            DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicRH(
                kShadowSceneW, kShadowSceneH, kShadowNearZ, kShadowFarZ);
            sc.mainLightViewProj = DirectX::XMMatrixMultiply(lightView, lightProj);
        }

		// ShadowMapの取得
        auto shadowRT = context.GetRenderTarget(ConstRenderPref::ShadowMap);
        if (shadowRT && shadowRT->GetSRVHandle())
            sc.shadowSrv = shadowRT->GetSRVHandle()->gpuHandle;
        
			context.SetShadowContext(sc);
    }

	// Geometry Pass
	for (auto& pass : m_framePasses.geometries)
		pass->Execute(context);

	// Lighting Pass
	for (auto& pass : m_framePasses.lightings)
	{
		if (pass.get() == m_ssrCompositePass.get())
		{
			auto sceneColorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
			if (sceneColorRT)
				context.SetDestRT(sceneColorRT);
		}
		pass->Execute(context);
	}

	// Transparent Pass
	for (auto& pass : m_framePasses.transparents)
		pass->Execute(context);

	// PostProcess Pass
	ExecutePostProcessChain(context);

	// Debug Pass
	for (auto& pass : m_framePasses.debugs)
	{
		if (pass.get() == m_debugPass.get())
		{
			auto sceneColorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
			auto sceneDepthRT = context.GetRenderTarget(ConstRenderPref::SceneDepth);
			if (sceneColorRT && sceneDepthRT)
			{
				context.SetSourceRT(sceneColorRT);
				context.SetDestRT(sceneDepthRT);
			}
		}

		pass->Execute(context);
	}

    // MSAA Resolve処理（フォワード時かつ MSAA 有効時のみ。デファード時は LightingPass が SceneColor に直接描画済み）
    if (m_aaSettings.msaaEnabled && !m_useDeferred)
    {
        RendererUtility::ResolveMSAA(context, ConstRenderPref::MSAART, ConstRenderPref::SceneColor);
        
        // モーションベクターバッファもResolve（TAA有効時）
        if (m_aaSettings.taaEnabled && m_motionVectorBuffer && m_motionVectorResolved)
        {
            // MSAAモーションベクターバッファを非MSAAバッファにResolve
            auto cmdList = context.CommandList;
            
            // Resolve用の状態遷移
            D3D12_RESOURCE_BARRIER barriers[2] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorBuffer->GetResource(), 
                    m_motionVectorBuffer->GetCurrentState(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorResolved->GetResource(), 
                    m_motionVectorResolved->GetCurrentState(), D3D12_RESOURCE_STATE_RESOLVE_DEST)
            };
            cmdList->ResourceBarrier(2, barriers);
            
            // Resolve
            cmdList->ResolveSubresource(
                m_motionVectorResolved->GetResource(), 0,
                m_motionVectorBuffer->GetResource(), 0,
                DXGI_FORMAT_R16G16_FLOAT);
            
            // Resolve後の状態遷移
            D3D12_RESOURCE_BARRIER barriersAfter[2] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorBuffer->GetResource(), 
                    D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
                CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorResolved->GetResource(), 
                    D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            };
            cmdList->ResourceBarrier(2, barriersAfter);
            
			// MotionVectorBufferとMotionVectorResolvedの状態遷移
            m_motionVectorBuffer->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_motionVectorResolved->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            
            context.AddRenderTarget(ConstRenderPref::MotionVectorBuffer, m_motionVectorResolved);
        }
    }

	m_tempRenderTargetPool->Return(tmpColorA);
	m_tempRenderTargetPool->Return(tmpColorB);
}

void DefaultPipelineManager::SetRayTracedShadowEnabled(bool enabled)
{
	if (m_rayTracedShadowPass)
	{
		m_rayTracedShadowPass->SetEnabled(enabled);
	}
}

bool DefaultPipelineManager::IsRayTracedShadowEnabled() const
{
	if (m_rayTracedShadowPass)
	{
		return m_rayTracedShadowPass->IsEnabled();
	}
	return false;
}

void DefaultPipelineManager::SetRayTracedAOEnabled(bool enabled)
{
	if (m_rayTracedAOPass)
		m_rayTracedAOPass->SetEnabled(enabled);
}

bool DefaultPipelineManager::IsRayTracedAOEnabled() const
{
	if (m_rayTracedAOPass)
		return m_rayTracedAOPass->IsEnabled();
	return false;
}

void DefaultPipelineManager::SetRayTracedGIEnabled(bool enabled)
{
	if (m_rayTracedGIPass)
		m_rayTracedGIPass->SetEnabled(enabled);
}

bool DefaultPipelineManager::IsRayTracedGIEnabled() const
{
	if (m_rayTracedGIPass)
		return m_rayTracedGIPass->IsEnabled();
	return false;
}

void DefaultPipelineManager::SetRayTracedReflectionEnabled(bool enabled)
{
	if (m_rayTracedReflectionPass)
		m_rayTracedReflectionPass->SetEnabled(enabled);
}

bool DefaultPipelineManager::IsRayTracedReflectionEnabled() const
{
	if (m_rayTracedReflectionPass)
		return m_rayTracedReflectionPass->IsEnabled();
	return false;
}

void DefaultPipelineManager::SetRTAODenoiseMode(RTAODenoiseMode mode)
{
	if (m_rtaoDenoisePass)
		m_rtaoDenoisePass->SetDenoiseMode(mode);
}

RTAODenoiseMode DefaultPipelineManager::GetRTAODenoiseMode() const
{
	if (m_rtaoDenoisePass)
		return m_rtaoDenoisePass->GetDenoiseMode();
	return RTAODenoiseMode::Off;
}

void DefaultPipelineManager::SetRTGIDenoiseMode(RTAODenoiseMode mode)
{
	if (m_rtgiDenoisePass)
		m_rtgiDenoisePass->SetDenoiseMode(mode);
}

RTAODenoiseMode DefaultPipelineManager::GetRTGIDenoiseMode() const
{
	if (m_rtgiDenoisePass)
		return m_rtgiDenoisePass->GetDenoiseMode();
	return RTAODenoiseMode::Off;
}

std::shared_ptr<ITargetBase> DefaultPipelineManager::GetRenderTargetForPreview(const char* name, DefaultScene* scene) const
{
	// 名前がnullptrならnullptrを返す
	if (!name) return nullptr;

	// 各種RenderTargetの取得
	if (strcmp(name, ConstRenderPref::SceneColor) == 0) return m_sceneColor;
	if (strcmp(name, ConstRenderPref::SceneDepth) == 0) return m_sceneDepth;
	if (strcmp(name, ConstRenderPref::ShadowMap) == 0) return m_shadowDepth;
	if (strcmp(name, ConstRenderPref::NormalBuffer) == 0) return m_normalBufferNonMSAA;
	if (strcmp(name, ConstRenderPref::WorldPositionBuffer) == 0) return m_worldPositionBufferNonMSAA;
	if (strcmp(name, ConstRenderPref::GBufferAlbedo) == 0) return m_gbufferAlbedo;
	if (strcmp(name, ConstRenderPref::GBufferMaterial) == 0) return m_gbufferMaterial;
	if (strcmp(name, ConstRenderPref::GBufferEmissive) == 0) return m_gbufferEmissive;
	if (strcmp(name, ConstRenderPref::SSAOBuffer) == 0)
		return (m_rayTracedAOPass && m_rayTracedAOPass->IsEnabled()) ? m_rtaoDenoiseBuffer : m_ssaoBuffer;

	// RTAORawの取得
	if (strcmp(name, ConstRenderPref::RTAORaw) == 0)
	{
		if (scene && m_rayTracedAOPass && m_rayTracedAOPass->IsEnabled())
		{
			auto shadowMgr = scene->GetRayTracedShadowManager();
			auto aoMgr = scene->GetRayTracedAOManager();
			if (shadowMgr && aoMgr && aoMgr->IsValid())
			{
				auto aoData = aoMgr->GetRenderData(g_Engine->CurrentBackBufferIndex(), shadowMgr->GetASManager());
				return aoData.aoTarget;
			}
		}
		return nullptr;
	}

	// RTAODenoiseTempの取得
	if (strcmp(name, ConstRenderPref::RTAODenoiseTemp) == 0) return m_rtaoDenoiseTemp;
	
	// RTGIBufferの取得
	if (strcmp(name, ConstRenderPref::RTGIBuffer) == 0)
	{
		if (m_rayTracedGIPass && m_rayTracedGIPass->IsEnabled())
		{
			if (GetRTGIDenoiseMode() != RTAODenoiseMode::Off)
				return m_rtgiDenoisedBuffer;
			if (scene)
			{
				auto shadowMgr = scene->GetRayTracedShadowManager();
				auto giMgr = scene->GetRayTracedGIManager();
				if (shadowMgr && giMgr && giMgr->IsValid())
				{
					auto giData = giMgr->GetRenderData(g_Engine->CurrentBackBufferIndex(), shadowMgr->GetASManager());
					return giData.giTarget;
				}
			}
		}
		return nullptr;
	}

	// RTGIRawの取得
	if (strcmp(name, ConstRenderPref::RTGIRaw) == 0)
	{
		if (scene && m_rayTracedGIPass && m_rayTracedGIPass->IsEnabled())
		{
			auto shadowMgr = scene->GetRayTracedShadowManager();
			auto giMgr = scene->GetRayTracedGIManager();
			if (shadowMgr && giMgr && giMgr->IsValid())
			{
				auto giData = giMgr->GetRenderData(g_Engine->CurrentBackBufferIndex(), shadowMgr->GetASManager());
				return giData.giTarget;
			}
		}
		return nullptr;
	}

	// RTGIDenoiseTempの取得
	if (strcmp(name, ConstRenderPref::RTGIDenoiseTemp) == 0) return m_rtgiDenoiseTemp;

	// SSRBufferの取得
	if (strcmp(name, ConstRenderPref::SSRBuffer) == 0) return m_ssrBuffer;

	// MotionVectorBufferの取得
	if (strcmp(name, ConstRenderPref::MotionVectorBuffer) == 0) return m_motionVectorResolved;
	
	return nullptr;
}

