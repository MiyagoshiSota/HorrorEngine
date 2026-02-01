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
		ImGui::SetNextWindowSizeConstraints(ImVec2(220.0f, 120.0f), ImVec2(FLT_MAX, FLT_MAX));

		if (!ImGui::Begin("Anti-Aliasing", &m_isVisible))
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

		ImGui::End();
	}

private:
};
