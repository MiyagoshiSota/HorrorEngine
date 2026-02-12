#pragma once

#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include <string>

/// アンチエイリアシング設定を一つのウィンドウに集約
class DrawAAWindow : public IDrawWindow
{
public:
	void draw() override
	{
		ImGui::SetNextWindowSizeConstraints(ImVec2(260.0f, 160.0f), ImVec2(FLT_MAX, FLT_MAX));

		if (!ImGui::Begin("Rendering / Anti-Aliasing", &m_isVisible))
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

		AASettings& aa = pipeline->GetAASettings();

		// --- デファード / フォワード ---
		bool useDeferred = pipeline->IsDeferredRendering();
		if (ImGui::Checkbox("Deferred Rendering", &useDeferred))
			pipeline->SetDeferredRendering(useDeferred);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("ON: G-Buffer + LightingPass (PBR). OFF: Forward (SimplePS 1-pass, MSAA possible).");

		ImGui::Separator();

		// --- MSAA (サンプリング型) ---
		bool msaa = aa.msaaEnabled;
		if (ImGui::Checkbox("MSAA", &msaa))
			pipeline->SetMSAAEnabled(msaa);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Sampling type AA (8x fixed).");

		// --- FXAA (ポストプロセス型) ---
		bool fxaa = aa.fxaaEnabled;
		if (ImGui::Checkbox("FXAA", &fxaa))
			pipeline->SetFXAAEnabled(fxaa);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Post-process AA. Combines with MSAA.");

		// --- TAA (時間軸型) ---
		bool taa = aa.taaEnabled;
		if (ImGui::Checkbox("TAA", &taa))
			pipeline->SetTAAEnabled(taa);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Temporal AA. Combines with MSAA/FXAA.");

		ImGui::Separator();

		// --- Ray Traced Shadow ---
		bool rayTracedShadow = pipeline->IsRayTracedShadowEnabled();
		if (ImGui::Checkbox("Ray Traced Shadow (DXR)", &rayTracedShadow))
			pipeline->SetRayTracedShadowEnabled(rayTracedShadow);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Enable hardware ray tracing for hard shadows. Requires DXR support.");

		// --- RTAO (Ray Traced Ambient Occlusion) ---
		bool rtao = pipeline->IsRayTracedAOEnabled();
		if (ImGui::Checkbox("RTAO (DXR)", &rtao))
			pipeline->SetRayTracedAOEnabled(rtao);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Ray traced ambient occlusion. Replaces SSAO when enabled. Requires DXR support.");

		// --- RTGI (Ray Traced Global Illumination) ---
		bool rtgi = pipeline->IsRayTracedGIEnabled();
		if (ImGui::Checkbox("RTGI (DXR)", &rtgi))
			pipeline->SetRayTracedGIEnabled(rtgi);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Ray traced global illumination (1-bounce indirect). Requires DXR support.");

		// --- RT Reflection (Ray Traced Reflection) ---
		bool rtReflection = pipeline->IsRayTracedReflectionEnabled();
		if (ImGui::Checkbox("RT Reflection (DXR)", &rtReflection))
			pipeline->SetRayTracedReflectionEnabled(rtReflection);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Ray traced mirror reflection from G-Buffer. Requires DXR support.");
		auto reflMgr = scene->GetRayTracedReflectionManager();
		if (reflMgr)
		{
			bool debugReflColors = reflMgr->IsDebugGeometryColors();
			if (ImGui::Checkbox("RT Reflection: Debug geometry colors", &debugReflColors))
				reflMgr->SetDebugGeometryColors(debugReflColors);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Bind distinct solid colors per geometry to verify SBT hit group selection.");
		}

		// --- RTAO Denoise Mode (RTAO有効時のみ意味がある) ---
		const char* denoiseItems[] = { "Off (copy)", "Bilateral (5x5)", "Separable (2-pass)", "A-Trous (5-pass)" };
		int denoiseIndex = static_cast<int>(pipeline->GetRTAODenoiseMode());
		if (ImGui::Combo("RTAO Denoise", &denoiseIndex, denoiseItems, 4))
			pipeline->SetRTAODenoiseMode(static_cast<RTAODenoiseMode>(denoiseIndex));
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Off: raw AO. Bilateral: 5x5 bilateral. Separable: 2-pass bilateral. A-Trous: 5-pass wavelet.");

		// --- SSAO ---
		bool ssao = pipeline->IsSSAOEnabled();
		if (ImGui::Checkbox("SSAO", &ssao))
			pipeline->SetSSAOEnabled(ssao);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Screen-space ambient occlusion. Ignored when RTAO is enabled.");

		// --- SSR (Screen-Space Reflection) ---
		bool ssr = pipeline->IsSSREnabled();
		if (ImGui::Checkbox("SSR", &ssr))
			pipeline->SetSSREnabled(ssr);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Screen-space reflection. Blends reflection over SceneColor after lighting.");

		ImGui::End();
	}

private:
};
