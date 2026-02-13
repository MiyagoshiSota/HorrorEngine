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
	// SimpleShadowMapPassの初期化
	m_simpleShadowMapPass = std::make_shared<SimpleShadowMapPass>();
	//m_simpleShadowMapPass = std::make_shared<CascadesShadowMapPass>();

	// Ray Traced Shadow Pass（リソースはSceneのRayTracedShadowManagerが所有）
	if (g_Engine->IsDxrSupported())
	{
		m_rayTracedShadowPass = std::make_shared<RayTracedShadowPass>();
		m_rayTracedShadowPass->SetEnabled(false); // デフォルトは無効
		m_rayTracedAOPass = std::make_shared<RTAOPass>();
		m_rayTracedAOPass->SetEnabled(false); // デフォルトは無効
		m_rayTracedGIPass = std::make_shared<RTGIPass>();
		m_rayTracedGIPass->SetEnabled(false); // デフォルトは無効
		m_rayTracedReflectionPass = std::make_shared<RTReflectionPass>();
		m_rayTracedReflectionPass->SetEnabled(false); // デフォルトは無効
	}

	// ParticleSystemの初期化
	m_rainParticleSystem = std::make_shared<RainParticleSystem>();
	
	// SkyboxPassの初期化
	m_skyboxPass = std::make_shared<SkyboxPass>();

    // Passを追加
    AddRenderProcessPass(std::make_shared<GeometryPass>());
	// AddRenderProcessPass(std::make_shared<DebugPass>());

    // ターゲットの生成
    m_sceneColor = std::make_shared<RenderTarget>();
	m_tmpColorA = std::make_shared<RenderTarget>();
	m_tmpColorB = std::make_shared<RenderTarget>();
	m_msaaTarget = std::make_shared<RenderTarget>();
	m_shadowDepth = std::make_shared<DepthStencilTarget>();
	m_cascadedShadowDepth = std::make_shared<DepthStencilTarget>();
	m_msaaDepth = std::make_shared<DepthStencilTarget>();
    m_sceneDepth = std::make_shared<DepthStencilTarget>();

    m_sceneColor->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM,1,1,1,0, g_Engine->AllocateRtvHandle(),g_Engine->GetDescriptorHeap()->Allocate(1));
	m_tmpColorA->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
    m_tmpColorB->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_historyBuffer = std::make_shared<RenderTarget>();
	m_historyBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	
	// モーションベクターバッファ（MSAA対応版と非MSAA版）
	m_motionVectorBuffer = std::make_shared<RenderTarget>();
	m_motionVectorBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16_FLOAT, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_motionVectorResolved = std::make_shared<RenderTarget>();
	m_motionVectorResolved->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	
	m_msaaTarget->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	// レイトレ用MRT：法線・ワールド位置（MSAA版と非MSAA版）
	m_normalBuffer = std::make_shared<RenderTarget>();
	m_worldPositionBuffer = std::make_shared<RenderTarget>();
	m_normalBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_worldPositionBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_normalBufferNonMSAA = std::make_shared<RenderTarget>();
	m_worldPositionBufferNonMSAA = std::make_shared<RenderTarget>();
	m_normalBufferNonMSAA->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_worldPositionBufferNonMSAA->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_gbufferAlbedo = std::make_shared<RenderTarget>();
	m_gbufferAlbedo->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_gbufferMaterial = std::make_shared<RenderTarget>();
	m_gbufferMaterial->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_gbufferEmissive = std::make_shared<RenderTarget>();
	m_gbufferEmissive->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_ssaoBuffer = std::make_shared<RenderTarget>();
	m_ssaoBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtaoDenoiseBuffer = std::make_shared<RenderTarget>();
	m_rtaoDenoiseBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtaoDenoiseTemp = std::make_shared<RenderTarget>();
	m_rtaoDenoiseTemp->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_ssrBuffer = std::make_shared<RenderTarget>();
	m_ssrBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtgiDenoisedBuffer = std::make_shared<RenderTarget>();
	m_rtgiDenoisedBuffer->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_rtgiDenoiseTemp = std::make_shared<RenderTarget>();
	m_rtgiDenoiseTemp->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 0, g_Engine->AllocateRtvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	m_shadowDepth->Create(g_Engine->Device(), 2048, 2048, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
    m_sceneDepth->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	// HACK:Depthの数が決め打ちになってるのでPass内のカスケードの数と合わせる
	m_cascadedShadowDepth->Create(g_Engine->Device(), 2048, 2048, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 3, 1, 1, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(3));
    m_msaaDepth->Create(g_Engine->Device(), kWindowWidth, kWindowHeight, DXGI_FORMAT_R32_TYPELESS, DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R32_FLOAT, 1, 1, AASettings::kMSAASampleCount, 0, g_Engine->AllocateDsvHandle(), g_Engine->GetDescriptorHeap()->Allocate(1));
	// PostProcessManagerの初期化
	m_postProcessManager = std::make_shared<PostProcessManager>();
    m_postProcessManager->LoadPresets(ConstPathPref::kPostProcessPresetsPath);
	m_postProcessManager->Init();

	m_fxaaPass = std::make_shared<FXAAPass>();
	m_taaPass = std::make_shared<TAAPass>();
	m_unjitterPass = std::make_shared<UnjitterPass>();

	// 一時レンダーターゲットプールの生成
	m_tempRenderTargetPool = std::make_shared<TempRenderTargetPool>();
	m_lightingPass = std::make_shared<LightingPass>();
	m_ssaoPass = std::make_shared<SSAOPass>();
	m_rtaoDenoisePass = std::make_shared<RTAODenoisePass>();
	m_rtgiDenoisePass = std::make_shared<RTGIDenoisePass>();
	m_ssrPass = std::make_shared<SSRPass>();
	m_ssrCompositePass = std::make_shared<SSRCompositePass>();
}

void DefaultPipelineManager::Execute()
{
    // TAA有効時：フレームごとにジッターを更新
    if (m_aaSettings.taaEnabled && m_taaPass)
    {
        m_taaPass->UpdateJitter();
    }

    // コンテキストを生成
    RenderContext context(g_Engine->CommandList(),g_Scene->GetSceneCamera(),g_Scene->GetGameObjects(), m_sceneColor, m_sceneDepth, kWindowWidth,kWindowHeight,g_Scene->GetPipelineStateManager(),m_tempRenderTargetPool);

    // TAA有効時：ジッターをコンテキストに設定（ジオメトリパスで投影行列に適用される）
    if (m_aaSettings.taaEnabled && m_taaPass)
    {
        context.SetTAAJitter(m_taaPass->GetCurrentJitter(), true);
    }

    // レンダーターゲットを設定
    auto defaultScene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
    context.AddRenderTarget(ConstRenderPref::SceneColor, m_sceneColor);
    context.AddRenderTarget(ConstRenderPref::SceneDepth, m_sceneDepth);
	context.AddRenderTarget(ConstRenderPref::TmpColorA, m_tmpColorA);
    context.AddRenderTarget(ConstRenderPref::TmpColorB, m_tmpColorB);
	context.AddRenderTarget(ConstRenderPref::HistoryBuffer, m_historyBuffer);
	context.AddRenderTarget(ConstRenderPref::MSAART, m_msaaTarget);
	context.AddRenderTarget(ConstRenderPref::MSAA_Depth, m_msaaDepth);

	if (m_useDeferred)
	{
		// デファード: G-Buffer（1x）を登録。LightingPass が SceneColor に描画する
		context.AddRenderTarget(ConstRenderPref::GBufferAlbedo, m_gbufferAlbedo);
		context.AddRenderTarget(ConstRenderPref::GBufferMaterial, m_gbufferMaterial);
		context.AddRenderTarget(ConstRenderPref::GBufferEmissive, m_gbufferEmissive);
		context.AddRenderTarget(ConstRenderPref::MotionVectorBuffer, m_motionVectorResolved);
		context.AddRenderTarget(ConstRenderPref::NormalBuffer, m_normalBufferNonMSAA);
		context.AddRenderTarget(ConstRenderPref::WorldPositionBuffer, m_worldPositionBufferNonMSAA);
		// RTAO有効時はRTAO raw + デノイズ出力、無効時は従来のSSAOバッファ
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
		// RTGI有効時はContextにRTGI出力を登録（LightingPassで使用）
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
		// RT Reflection有効時はContextにRT Reflection出力を登録（LightingPassで使用）
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
		// SSR用バッファを登録（SSRPass が書き込み、SSRCompositePass が参照）
		context.AddRenderTarget(ConstRenderPref::SSRBuffer, m_ssrBuffer);
	}
	else
	{
		// フォワード: GeometryPass の 4 RTV 用。MSAA 有無でバッファを切り替え
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

	// ShadowMapターゲットの設定（Ray Traced Shadowが有効な場合はそちらを優先）
	if (defaultScene && m_rayTracedShadowPass && m_rayTracedShadowPass->IsEnabled())
	{
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
	context.AddRenderTarget(ConstRenderPref::CascadedShadowMap, m_cascadedShadowDepth);

    if (defaultScene)
    {
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

            // 定数バッファ更新用のコールバックを設定
            context.SetSkyboxUpdateCallback([skyboxManager](DirectX::XMMATRIX viewProj) {
                skyboxManager->UpdateConstantBuffer(viewProj);
            });
        }
    }

    // Ray Traced Shadow用データをContextに設定
    if (defaultScene)
    {
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
        // RTAO用データをContextに設定（TLASはShadowと共有）
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
        // RTGI用データをContextに設定
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
        // RT Reflection用データをContextに設定
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

    // Shadow（ライト空間深度 or R32 マスク）
    if (m_rayTracedShadowPass && m_rayTracedShadowPass->IsEnabled())
    {
        m_rayTracedShadowPass->Execute(context);
    }
    else
    {
        m_simpleShadowMapPass->LastExecute(context);
    }

    // ShadowContext を構築（LightingPass が参照）
    {
        ShadowContext sc;
        sc.mode = (m_rayTracedShadowPass && m_rayTracedShadowPass->IsEnabled())
            ? ShadowMode::RayTracedMask
            : ShadowMode::RasterDepth;
        const float kShadowSceneW = 50.0f;
        const float kShadowSceneH = 50.0f;
        const float kShadowNearZ = 0.0f;
        const float kShadowFarZ = 10.0f;
        const float kShadowLightDist = 25.0f;
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
        auto shadowRT = context.GetRenderTarget(ConstRenderPref::ShadowMap);
        if (shadowRT && shadowRT->GetSRVHandle())
            sc.shadowSrv = shadowRT->GetSRVHandle()->gpuHandle;
        context.SetShadowContext(sc);
    }

    // Mesh（G-Buffer 出力）
    for (auto& pass : m_sceneRenderPasses)
    {
        pass->Execute(context);
    }

    // SSAO または RTAO（デファード時のみ）
    if (m_useDeferred)
    {
        if (m_rayTracedAOPass && m_rayTracedAOPass->IsEnabled())
        {
            m_rayTracedAOPass->Execute(context);
            if (m_rtaoDenoisePass)
                m_rtaoDenoisePass->Execute(context);
        }
        else if (m_ssaoPass)
        {
            m_ssaoPass->SetEnabled(m_ssaoEnabled);
            m_ssaoPass->Execute(context);
        }
        // RTGI（デファード時のみ、ON/OFF可能）
        if (m_rayTracedGIPass && m_rayTracedGIPass->IsEnabled())
        {
            m_rayTracedGIPass->Execute(context);
            if (m_rtgiDenoisePass && GetRTGIDenoiseMode() != RTAODenoiseMode::Off)
                m_rtgiDenoisePass->Execute(context);
        }
        // RT Reflection（デファード時のみ、ON/OFF可能）
        if (m_rayTracedReflectionPass && m_rayTracedReflectionPass->IsEnabled())
            m_rayTracedReflectionPass->Execute(context);
    }

    // Lighting（デファード時のみ: G-Buffer + Shadow + SSAO + RTGI + RT Reflection → SceneColor）
    if (m_useDeferred && m_lightingPass)
    {
        context.SetRTGIEnabled(m_rayTracedGIPass && m_rayTracedGIPass->IsEnabled());
        context.SetRTReflectionEnabled(m_rayTracedReflectionPass && m_rayTracedReflectionPass->IsEnabled());
        m_lightingPass->Execute(context);
    }

    // SSR（デファード時のみ: SceneColor + Depth + G-Buffer → SSRBuffer → TmpColorA → SceneColor）
    if (m_useDeferred && m_ssrPass && m_ssrCompositePass && m_ssrEnabled)
    {
        auto cmdList = context.CommandList;
        auto sceneColorRT = context.GetRenderTarget(ConstRenderPref::SceneColor);
        auto sceneDepthRT = context.GetRenderTarget(ConstRenderPref::SceneDepth);
        if (sceneColorRT && sceneDepthRT && sceneColorRT->GetResource() && sceneDepthRT->GetResource())
        {
            if (sceneColorRT->GetCurrentState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            {
                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    sceneColorRT->GetResource(),
                    sceneColorRT->GetCurrentState(),
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                cmdList->ResourceBarrier(1, &barrier);
                sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
            if (sceneDepthRT->GetCurrentState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            {
                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    sceneDepthRT->GetResource(),
                    sceneDepthRT->GetCurrentState(),
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                cmdList->ResourceBarrier(1, &barrier);
                sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
            m_ssrPass->Execute(context);
            context.SetDestRT(m_tmpColorA);
            m_ssrCompositePass->Execute(context);
            if (m_tmpColorA->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            {
                D3D12_RESOURCE_BARRIER barriers[2] = {
                    CD3DX12_RESOURCE_BARRIER::Transition(m_tmpColorA->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
                    CD3DX12_RESOURCE_BARRIER::Transition(m_sceneColor->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
                };
                cmdList->ResourceBarrier(2, barriers);
                m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
                m_sceneColor->SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
                cmdList->CopyResource(m_sceneColor->GetResource(), m_tmpColorA->GetResource());
                D3D12_RESOURCE_BARRIER barriersBack[2] = {
                    CD3DX12_RESOURCE_BARRIER::Transition(m_tmpColorA->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
                    CD3DX12_RESOURCE_BARRIER::Transition(m_sceneColor->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET)
                };
                cmdList->ResourceBarrier(2, barriersBack);
                m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                m_sceneColor->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
            }
            if (sceneDepthRT->GetCurrentState() != D3D12_RESOURCE_STATE_DEPTH_WRITE)
            {
                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                    sceneDepthRT->GetResource(),
                    sceneDepthRT->GetCurrentState(),
                    D3D12_RESOURCE_STATE_DEPTH_WRITE);
                cmdList->ResourceBarrier(1, &barrier);
                sceneDepthRT->SetCurrentState(D3D12_RESOURCE_STATE_DEPTH_WRITE);
            }
        }
    }

    // Skybox（SceneColor に描画）
    if (m_skyboxPass && m_skyboxPass->IsEnabled(context))
    {
        m_skyboxPass->Execute(context);
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
            
            // Resolve後の状態遷移（SRVとして使用するため）
            D3D12_RESOURCE_BARRIER barriersAfter[2] = {
                CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorBuffer->GetResource(), 
                    D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
                CD3DX12_RESOURCE_BARRIER::Transition(m_motionVectorResolved->GetResource(), 
                    D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            };
            cmdList->ResourceBarrier(2, barriersAfter);
            
            m_motionVectorBuffer->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_motionVectorResolved->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            
            // TAAで使用するのはResolvedバッファ
            context.AddRenderTarget(ConstRenderPref::MotionVectorBuffer, m_motionVectorResolved);
        }
    }

	// Particle
	m_rainParticleSystem->Execute(context);

    // PostProcess & AA
    ExecutePostProcess(context);
}

void DefaultPipelineManager::ExecutePostProcess(RenderContext& context)
{
	// FXAA/TAA 有効時は中間バッファに一度出力する
	const bool needsIntermediateBuffer = m_aaSettings.fxaaEnabled || m_aaSettings.taaEnabled;
	if (!needsIntermediateBuffer)
	{
		m_postProcessManager->ExecutePasses(context);
		return;
	}

	// 前フレームで PIXEL_SHADER_RESOURCE にしたままなので、RTV として使う前に RENDER_TARGET へ戻す
	if (m_tmpColorA->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_tmpColorA->GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);
		context.CommandList->ResourceBarrier(1, &barrier);
		m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	// ポストプロセスチェインを TmpColorA に出力
	m_postProcessManager->ExecutePasses(context, m_tmpColorA);

	// AA 入力として TmpColorA を SRV 状態に
	if (m_tmpColorA->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_tmpColorA->GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		context.CommandList->ResourceBarrier(1, &barrier);
		m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// TAA 優先（その後に FXAA を重ねる場合あり）
	if (m_aaSettings.taaEnabled)
	{
		ApplyTAA(context);
	}
	else if (m_aaSettings.fxaaEnabled)
	{
		ApplyFXAAAfterPostProcess(context, m_tmpColorA);
	}
}

void DefaultPipelineManager::ApplyTAA(RenderContext& context)
{
	// HistoryBuffer を PIXEL_SHADER_RESOURCE に（読み取り用）
	if (m_historyBuffer->GetCurrentState() != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_historyBuffer->GetResource(),
			m_historyBuffer->GetCurrentState(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		context.CommandList->ResourceBarrier(1, &barrier);
		m_historyBuffer->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// TAA 実行: TmpColorA + HistoryBuffer → TmpColorB
	context.SetSourceRT(m_tmpColorA);
	context.SetDestRT(m_tmpColorB);
	m_taaPass->Execute(context);

	// TAA 結果を SRV 状態に
	if (m_tmpColorB->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_tmpColorB->GetResource(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		context.CommandList->ResourceBarrier(1, &barrier);
		m_tmpColorB->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}

	// FXAA も有効なら TAA 結果に対して FXAA を適用
	if (m_aaSettings.fxaaEnabled)
	{
		ApplyFXAAAfterPostProcess(context, m_tmpColorB);
	}
	else
	{
		// TAA のみ: TAA 結果をバックバッファへコピー
		ID3D12Resource* backBuffer = g_Engine->GetCurrentBackBuffer();
		if (m_tmpColorB->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE && backBuffer)
		{
			D3D12_RESOURCE_BARRIER barriers[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(m_tmpColorB->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST)
			};
			context.CommandList->ResourceBarrier(2, barriers);
			m_tmpColorB->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
			context.CommandList->CopyResource(backBuffer, m_tmpColorB->GetResource());
			D3D12_RESOURCE_BARRIER barriersBack[2] = {
				CD3DX12_RESOURCE_BARRIER::Transition(m_tmpColorB->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
				CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET)
			};
			context.CommandList->ResourceBarrier(2, barriersBack);
			m_tmpColorB->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}

	// 履歴バッファ更新: TAA結果をUnjitterしてから保存
	// Step1: TmpColorB（TAA結果、ジッター適用済み）→ Unjitterパス → TmpColorA（Unjitter結果）
	if (m_tmpColorB->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		// TmpColorAをRTVとして準備
		if (m_tmpColorA->GetCurrentState() != D3D12_RESOURCE_STATE_RENDER_TARGET)
		{
			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_tmpColorA->GetResource(),
				m_tmpColorA->GetCurrentState(),
				D3D12_RESOURCE_STATE_RENDER_TARGET);
			context.CommandList->ResourceBarrier(1, &barrier);
			m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
		}

		// Unjitterパス実行: TmpColorB → TmpColorA
		m_unjitterPass->SetJitter(m_taaPass->GetCurrentJitter());
		context.SetSourceRT(m_tmpColorB);
		context.SetDestRT(m_tmpColorA);
		m_unjitterPass->Execute(context);

		// TmpColorA（Unjitter結果）をSRV状態に
		if (m_tmpColorA->GetCurrentState() == D3D12_RESOURCE_STATE_RENDER_TARGET)
		{
			D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m_tmpColorA->GetResource(),
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context.CommandList->ResourceBarrier(1, &barrier);
			m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		// Step2: Unjitter結果（TmpColorA）を履歴バッファにコピー
		D3D12_RESOURCE_BARRIER barriers[2] = {
			CD3DX12_RESOURCE_BARRIER::Transition(m_tmpColorA->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_historyBuffer->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST)
		};
		context.CommandList->ResourceBarrier(2, barriers);
		m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_historyBuffer->SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);
		context.CommandList->CopyResource(m_historyBuffer->GetResource(), m_tmpColorA->GetResource());
		D3D12_RESOURCE_BARRIER barriersBack[2] = {
			CD3DX12_RESOURCE_BARRIER::Transition(m_tmpColorA->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(m_historyBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		};
		context.CommandList->ResourceBarrier(2, barriersBack);
		m_tmpColorA->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_historyBuffer->SetCurrentState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
}

void DefaultPipelineManager::ApplyFXAAAfterPostProcess(RenderContext& context, std::shared_ptr<ITargetBase> sourceRT)
{
	context.SetSourceRT(sourceRT);
	m_fxaaPass->LastExecute(context, g_Engine->GetCurrentRtvHandle());
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
	if (!name) return nullptr;
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
	if (strcmp(name, ConstRenderPref::RTAODenoiseTemp) == 0) return m_rtaoDenoiseTemp;
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
	if (strcmp(name, ConstRenderPref::RTGIDenoiseTemp) == 0) return m_rtgiDenoiseTemp;
	if (strcmp(name, ConstRenderPref::SSRBuffer) == 0) return m_ssrBuffer;
	if (strcmp(name, ConstRenderPref::MotionVectorBuffer) == 0) return m_motionVectorResolved;
	return nullptr;
}

