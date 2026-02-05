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

		// --- SSAO ---
		bool ssao = pipeline->IsSSAOEnabled();
		if (ImGui::Checkbox("SSAO", &ssao))
			pipeline->SetSSAOEnabled(ssao);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Screen-space ambient occlusion. Ignored when RTAO is enabled.");

		ImGui::End();
	}

private:
};
