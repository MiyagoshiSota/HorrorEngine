#pragma once

#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Scene/Default/Renderer/PipelineManager/DefaultPipelineManager.h"
#include <string>

/// レンダリング設定を1ウィンドウ＋タブで集約
/// タブ: RenderRing, AA, Shadow, AO, AO Denoise, Reflection, GI, GI Denoise
class DrawAAWindow : public IDrawWindow
{
public:
	void draw() override
	{
		ImGui::SetNextWindowSizeConstraints(ImVec2(280.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));

		if (!ImGui::Begin("Rendering Settings", &m_isVisible))
		{
			ImGui::End();
			return;
		}

		auto scene = std::dynamic_pointer_cast<DefaultScene>(g_Scene);
		if (scene == nullptr)
		{
			ImGui::Text("(Scene not ready)");
			ImGui::End();
			return;
		}

		auto pipeline = scene->GetDefaultPipelineManager();
		if (pipeline == nullptr)
		{
			ImGui::Text("(Pipeline not ready)");
			ImGui::End();
			return;
		}

		if (ImGui::BeginTabBar("RenderingTabs"))
		{
			DrawPipelineTab(pipeline);
			DrawRenderRingTab(pipeline);
			DrawAATab(pipeline);
			DrawShadowTab(pipeline);
			DrawAOTab(pipeline);
			DrawAODenoiseTab(pipeline);
			DrawReflectionTab(pipeline, scene);
			DrawGITab(pipeline);
			DrawGIDenoiseTab(pipeline);
			ImGui::EndTabBar();
		}

		ImGui::End();
	}

private:
	void DrawPipelineTab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("Pipeline"))
			return;

		const bool useDeferred = pipeline->IsDeferredRendering();
		const AASettings& aa = pipeline->GetAASettings();
		const bool rtxShadow = pipeline->IsRayTracedShadowEnabled();
		const bool rtao = pipeline->IsRayTracedAOEnabled();
		const bool ssao = pipeline->IsSSAOEnabled();
		const bool rtgi = pipeline->IsRayTracedGIEnabled();
		const bool rtr = pipeline->IsRayTracedReflectionEnabled();
		const bool ssr = pipeline->IsSSREnabled();

		const ImVec4 headerColor(0.6f, 0.8f, 1.0f, 1.0f);
		const ImVec4 activeColor(0.4f, 1.0f, 0.4f, 1.0f);
		const ImVec4 inactiveColor(0.5f, 0.5f, 0.5f, 1.0f);

		auto Section = [&headerColor](const char* name) {
			ImGui::Spacing();
			ImGui::TextColored(headerColor, "[%s]", name);
		};
		auto Pass = [&activeColor, &inactiveColor](const char* name, bool active) {
			ImGui::TextColored(active ? activeColor : inactiveColor, "  - %s%s", name, active ? "" : " (off)");
		};

		Section("1. Shadow");
		Pass(rtxShadow ? "RayTracedShadowPass" : "SimpleShadowMapPass", true);

		Section("2. Geometry");
		if (useDeferred)
			Pass("GeometryPass (GBufferPass)", true);
		else
			Pass(aa.msaaEnabled ? "GeometryPass (SimplePS + MSAA)" : "GeometryPass (SimplePS)", true);

		Section("3. Lighting");
		if (useDeferred)
		{
			if (rtao)
			{
				Pass("RTAOPass", true);
				Pass("RTAODenoisePass", true);
			}
			else
				Pass("SSAOPass", ssao);
			Pass("RTGIPass", rtgi);
			Pass("RTGIDenoisePass", rtgi && pipeline->GetRTGIDenoiseMode() != RTAODenoiseMode::Off);
			Pass("RTReflectionPass", rtr);
			Pass("LightingPass", true);
			if (ssr)
			{
				Pass("SSRPass", true);
				Pass("SSRCompositePass", true);
			}
		}
		else
		{
			Pass("(integrated in Geometry)", true);
		}

		Section("4. Composition");
		Pass("SkyboxPass", true);
		if (!useDeferred && aa.msaaEnabled)
			Pass("MSAA Resolve", true);
		Pass("RainParticleSystem", true);

		Section("5. PostProcess");
		Pass("PostProcessManager", true);
		Pass("TAAPass", aa.taaEnabled);
		Pass("FXAAPass", aa.fxaaEnabled);
		Pass("BackBuffer", true);

		ImGui::EndTabItem();
	}

	void DrawRenderRingTab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("RenderRing"))
			return;

		int mode = pipeline->IsDeferredRendering() ? 0 : 1;
		if (ImGui::RadioButton("Deferred", &mode, 0))
			pipeline->SetDeferredRendering(true);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("G-Buffer + LightingPass (PBR). 多光源向け。");

		if (ImGui::RadioButton("Forward", &mode, 1))
			pipeline->SetDeferredRendering(false);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("SimplePS 1-pass. MSAA 利用可。");

		ImGui::EndTabItem();
	}

	void DrawAATab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("AA"))
			return;

		const bool useDeferred = pipeline->IsDeferredRendering();
		AASettings& aa = pipeline->GetAASettings();

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Forward only ---");
		ImGui::BeginDisabled(useDeferred);
		bool msaa = aa.msaaEnabled;
		if (ImGui::Checkbox("MSAA (Forward)", &msaa))
			pipeline->SetMSAAEnabled(msaa);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("8x MSAA. Forward only. Deferred uses 1x G-Buffer.");
		ImGui::EndDisabled();
		if (useDeferred)
			ImGui::TextDisabled("  MSAA: Deferred uses 1x (N/A)");

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Both ---");
		bool fxaa = aa.fxaaEnabled;
		if (ImGui::Checkbox("FXAA", &fxaa))
			pipeline->SetFXAAEnabled(fxaa);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Post-process AA. Combines with MSAA.");

		auto fxaaPass = pipeline->GetFXAAPass();
		if (fxaaPass && fxaa)
		{
			float edgeThresh = fxaaPass->GetEdgeThreshold();
			if (ImGui::SliderFloat("FXAA Edge Threshold", &edgeThresh, 0.01f, 0.5f, "%.3f"))
				fxaaPass->SetEdgeThreshold(edgeThresh);
			float edgeThreshMin = fxaaPass->GetEdgeThresholdMin();
			if (ImGui::SliderFloat("FXAA Edge Threshold Min", &edgeThreshMin, 0.01f, 0.2f, "%.3f"))
				fxaaPass->SetEdgeThresholdMin(edgeThreshMin);
		}

		bool taa = aa.taaEnabled;
		if (ImGui::Checkbox("TAA", &taa))
			pipeline->SetTAAEnabled(taa);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Temporal AA. Combines with MSAA/FXAA.");

		auto taaPass = pipeline->GetTAAPass();
		if (taaPass && taa)
		{
			float blendFactor = taaPass->GetBlendFactor();
			if (ImGui::SliderFloat("TAA Blend Factor", &blendFactor, 0.01f, 0.5f, "%.3f"))
				taaPass->SetBlendFactor(blendFactor);
			float historyWeight = taaPass->GetHistoryWeight();
			if (ImGui::SliderFloat("TAA History Weight", &historyWeight, 0.5f, 0.99f, "%.3f"))
				taaPass->SetHistoryWeight(historyWeight);
		}

		ImGui::EndTabItem();
	}

	void DrawShadowTab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("Shadow"))
			return;

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Both (Deferred / Forward) ---");
		bool rayTracedShadow = pipeline->IsRayTracedShadowEnabled();
		if (ImGui::Checkbox("RT Shadow (DXR)", &rayTracedShadow))
			pipeline->SetRayTracedShadowEnabled(rayTracedShadow);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Hardware ray traced hard shadows. Requires DXR.");

		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("OFF = SimpleShadowMap (depth map).");

		ImGui::EndTabItem();
	}

	void DrawAOTab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("AO"))
			return;

		const bool useDeferred = pipeline->IsDeferredRendering();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Deferred only ---");
		ImGui::BeginDisabled(!useDeferred);
		if (!useDeferred)
			ImGui::TextDisabled("AO: G-Buffer required. Switch to Deferred.");

		bool rtao = pipeline->IsRayTracedAOEnabled();
		if (ImGui::Checkbox("RTAO (DXR)", &rtao))
			pipeline->SetRayTracedAOEnabled(rtao);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Ray traced ambient occlusion. Replaces SSAO when enabled.");

		auto rtaoPass = pipeline->GetRTAOPass();
		if (rtaoPass && rtao)
		{
			float radius = rtaoPass->GetRadius();
			if (ImGui::SliderFloat("RTAO Radius", &radius, 0.01f, 2.0f, "%.3f"))
				rtaoPass->SetRadius(radius);
			float bias = rtaoPass->GetBias();
			if (ImGui::SliderFloat("RTAO Bias", &bias, 0.0f, 0.1f, "%.4f"))
				rtaoPass->SetBias(bias);
			int numRays = static_cast<int>(rtaoPass->GetNumRaysPerPixel());
			if (ImGui::SliderInt("RTAO Rays/Pixel", &numRays, 1, 16))
				rtaoPass->SetNumRaysPerPixel(static_cast<UINT>(numRays));
		}

		bool ssao = pipeline->IsSSAOEnabled();
		if (ImGui::Checkbox("SSAO", &ssao))
			pipeline->SetSSAOEnabled(ssao);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Screen-space AO. Ignored when RTAO is enabled.");

		auto ssaoPass = pipeline->GetSSAOPass();
		if (ssaoPass && ssao)
		{
			float radius = ssaoPass->GetRadius();
			if (ImGui::SliderFloat("SSAO Radius", &radius, 0.1f, 2.0f, "%.3f"))
				ssaoPass->SetRadius(radius);
			float bias = ssaoPass->GetBias();
			if (ImGui::SliderFloat("SSAO Bias", &bias, 0.0f, 0.1f, "%.4f"))
				ssaoPass->SetBias(bias);
			float power = ssaoPass->GetPower();
			if (ImGui::SliderFloat("SSAO Power", &power, 0.5f, 4.0f, "%.2f"))
				ssaoPass->SetPower(power);
		}

		ImGui::EndDisabled();
		ImGui::EndTabItem();
	}

	void DrawAODenoiseTab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("AO Denoise"))
			return;

		const bool useDeferred = pipeline->IsDeferredRendering();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Deferred only (RTAO) ---");
		ImGui::BeginDisabled(!useDeferred);
		if (!useDeferred)
			ImGui::TextDisabled("RTAO Denoise: Deferred + RTAO required.");

		const char* denoiseItems[] = { "Off (copy)", "Bilateral (5x5)", "Bilateral Separable (2-pass)", "A-Trous (5-pass)" };
		int denoiseIndex = static_cast<int>(pipeline->GetRTAODenoiseMode());
		if (ImGui::Combo("RTAO Denoise", &denoiseIndex, denoiseItems, 4))
			pipeline->SetRTAODenoiseMode(static_cast<RTAODenoiseMode>(denoiseIndex));
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("RTAO用デノイザ。Off/Bilateral/Separable/A-Trous。");

		auto denoisePass = pipeline->GetRTAODenoisePass();
		if (denoisePass && pipeline->IsRayTracedAOEnabled())
		{
			float depthSigma = denoisePass->GetDepthSigma();
			if (ImGui::SliderFloat("Depth Sigma", &depthSigma, 1.0f, 64.0f, "%.1f"))
				denoisePass->SetDepthSigma(depthSigma);
			float normalSigma = denoisePass->GetNormalSigma();
			if (ImGui::SliderFloat("Normal Sigma", &normalSigma, 1.0f, 32.0f, "%.1f"))
				denoisePass->SetNormalSigma(normalSigma);
		}

		ImGui::EndDisabled();
		ImGui::EndTabItem();
	}

	void DrawReflectionTab(std::shared_ptr<DefaultPipelineManager> pipeline, std::shared_ptr<DefaultScene> scene)
	{
		if (!ImGui::BeginTabItem("Reflection"))
			return;

		const bool useDeferred = pipeline->IsDeferredRendering();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Deferred only ---");
		ImGui::BeginDisabled(!useDeferred);
		if (!useDeferred)
			ImGui::TextDisabled("Reflection: G-Buffer required. Switch to Deferred.");

		bool ssr = pipeline->IsSSREnabled();
		if (ImGui::Checkbox("SSR", &ssr))
			pipeline->SetSSREnabled(ssr);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Screen-space reflection.");

		auto ssrPass = pipeline->GetSSRPass();
		if (ssrPass && ssr)
		{
			float maxDist = ssrPass->GetMaxRayDistance();
			if (ImGui::SliderFloat("SSR Max Ray Distance", &maxDist, 5.0f, 200.0f, "%.0f"))
				ssrPass->SetMaxRayDistance(maxDist);
			float rayStep = ssrPass->GetRayStep();
			if (ImGui::SliderFloat("SSR Ray Step", &rayStep, 0.1f, 5.0f, "%.2f"))
				ssrPass->SetRayStep(rayStep);
			int maxSteps = ssrPass->GetMaxSteps();
			if (ImGui::SliderInt("SSR Max Steps", &maxSteps, 8, 128))
				ssrPass->SetMaxSteps(maxSteps);
			float thickness = ssrPass->GetThickness();
			if (ImGui::SliderFloat("SSR Thickness", &thickness, 0.01f, 0.5f, "%.3f"))
				ssrPass->SetThickness(thickness);
		}

		bool rtReflection = pipeline->IsRayTracedReflectionEnabled();
		if (ImGui::Checkbox("RTR (DXR)", &rtReflection))
			pipeline->SetRayTracedReflectionEnabled(rtReflection);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Ray traced reflection. Requires DXR.");

		auto rtrPass = pipeline->GetRayTracedReflectionPass();
		if (rtrPass && rtReflection)
		{
			float bias = rtrPass->GetBias();
			if (ImGui::SliderFloat("RTR Bias", &bias, 0.0f, 0.1f, "%.4f"))
				rtrPass->SetBias(bias);
			float maxDist = rtrPass->GetMaxDistance();
			if (ImGui::SliderFloat("RTR Max Distance", &maxDist, 10.0f, 500.0f, "%.0f"))
				rtrPass->SetMaxDistance(maxDist);
			float intensity = rtrPass->GetReflectionIntensity();
			if (ImGui::SliderFloat("RTR Intensity", &intensity, 0.0f, 2.0f, "%.2f"))
				rtrPass->SetReflectionIntensity(intensity);
			float roughnessThresh = rtrPass->GetRoughnessThreshold();
			if (ImGui::SliderFloat("RTR Roughness Thresh", &roughnessThresh, 0.0f, 1.0f, "%.2f"))
				rtrPass->SetRoughnessThreshold(roughnessThresh);
			float fresnelF0 = rtrPass->GetFresnelF0();
			if (ImGui::SliderFloat("RTR Fresnel F0", &fresnelF0, 0.0f, 1.0f, "%.3f"))
				rtrPass->SetFresnelF0(fresnelF0);
		}

		auto reflMgr = scene->GetRayTracedReflectionManager();
		if (reflMgr)
		{
			bool debugReflColors = reflMgr->IsDebugGeometryColors();
			if (ImGui::Checkbox("RTR: Debug geometry colors", &debugReflColors))
				reflMgr->SetDebugGeometryColors(debugReflColors);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("SBT hit group verification.");
		}

		ImGui::EndDisabled();
		ImGui::EndTabItem();
	}

	void DrawGITab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("GI"))
			return;

		const bool useDeferred = pipeline->IsDeferredRendering();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Deferred only ---");
		ImGui::BeginDisabled(!useDeferred);
		if (!useDeferred)
			ImGui::TextDisabled("RT GI: G-Buffer required. Switch to Deferred.");

		bool rtgi = pipeline->IsRayTracedGIEnabled();
		if (ImGui::Checkbox("RT GI (Path Tracing)", &rtgi))
			pipeline->SetRayTracedGIEnabled(rtgi);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Ray traced global illumination (1-bounce indirect). Requires DXR.");

		auto rtgiPass = pipeline->GetRayTracedGIPass();
		if (rtgiPass && rtgi)
		{
			float radius = rtgiPass->GetRadius();
			if (ImGui::SliderFloat("RTGI Radius", &radius, 0.1f, 5.0f, "%.2f"))
				rtgiPass->SetRadius(radius);
			float bias = rtgiPass->GetBias();
			if (ImGui::SliderFloat("RTGI Bias", &bias, 0.0f, 0.1f, "%.4f"))
				rtgiPass->SetBias(bias);
			float indirectIntensity = rtgiPass->GetIndirectIntensity();
			if (ImGui::SliderFloat("RTGI Indirect Intensity", &indirectIntensity, 0.0f, 3.0f, "%.2f"))
				rtgiPass->SetIndirectIntensity(indirectIntensity);
			int numRays = static_cast<int>(rtgiPass->GetNumRaysPerPixel());
			if (ImGui::SliderInt("RTGI Rays/Pixel", &numRays, 1, 16))
				rtgiPass->SetNumRaysPerPixel(static_cast<UINT>(numRays));
		}

		ImGui::EndDisabled();
		ImGui::EndTabItem();
	}

	void DrawGIDenoiseTab(std::shared_ptr<DefaultPipelineManager> pipeline)
	{
		if (!ImGui::BeginTabItem("GI Denoise"))
			return;

		const bool useDeferred = pipeline->IsDeferredRendering();
		const bool rtgi = pipeline->IsRayTracedGIEnabled();
		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "--- Deferred + RT GI only ---");
		ImGui::BeginDisabled(!useDeferred || !rtgi);
		if (!useDeferred)
			ImGui::TextDisabled("RT GI Denoise: Deferred required.");
		else if (!rtgi)
			ImGui::TextDisabled("RT GI Denoise: Enable RT GI first.");

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "RTGI Denoise (AO同様のjoint bilateral)");
		const char* giDenoiseItems[] = { "Off (raw)", "Bilateral (5x5)", "Bilateral Separable (2-pass)", "A-Trous (5-pass)" };
		int giDenoiseIndex = static_cast<int>(pipeline->GetRTGIDenoiseMode());
		if (ImGui::Combo("RTGI Denoise", &giDenoiseIndex, giDenoiseItems, 4))
			pipeline->SetRTGIDenoiseMode(static_cast<RTAODenoiseMode>(giDenoiseIndex));
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("RTGI用デノイザ。Off/Bilateral/Separable/A-Trous。");

		auto rtgiDenoisePass = pipeline->GetRTGIDenoisePass();
		if (rtgiDenoisePass && rtgi && pipeline->GetRTGIDenoiseMode() != RTAODenoiseMode::Off)
		{
			float depthSigma = rtgiDenoisePass->GetDepthSigma();
			if (ImGui::SliderFloat("Depth Sigma", &depthSigma, 1.0f, 64.0f, "%.1f"))
				rtgiDenoisePass->SetDepthSigma(depthSigma);
			float normalSigma = rtgiDenoisePass->GetNormalSigma();
			if (ImGui::SliderFloat("Normal Sigma", &normalSigma, 1.0f, 32.0f, "%.1f"))
				rtgiDenoisePass->SetNormalSigma(normalSigma);
		}

		ImGui::EndDisabled();
		ImGui::EndTabItem();
	}
};
