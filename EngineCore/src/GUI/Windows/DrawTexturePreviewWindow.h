#pragma once

#include "GUI/Core/IDrawWindow.h"
#include "imgui.h"
#include "Core/App.h"
#include "Renderer/Engine.h"
#include "Scene/Default/Scene/DefaultScene.h"
#include "Scene/Default/Renderer/PipelineManager/DefaultPipelineManager.h"
#include "Modules/PublicConst/ConstRenderPref.h"

/// 各パスの入出力テクスチャをプレビュー表示
class DrawTexturePreviewWindow : public IDrawWindow
{
public:
	void draw() override
	{
		ImGui::SetNextWindowSize(ImVec2(420.0f, 480.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Texture Preview", &m_isVisible))
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

		static const char* kTextureNames[] = {
			ConstRenderPref::SceneColor,
			ConstRenderPref::SceneDepth,
			ConstRenderPref::ShadowMap,
			ConstRenderPref::NormalBuffer,
			ConstRenderPref::WorldPositionBuffer,
			ConstRenderPref::GBufferAlbedo,
			ConstRenderPref::GBufferMaterial,
			ConstRenderPref::GBufferEmissive,
			ConstRenderPref::SSAOBuffer,
			ConstRenderPref::RTAORaw,
			ConstRenderPref::RTGIBuffer,
			ConstRenderPref::RTGIRaw,
			ConstRenderPref::SSRBuffer,
			ConstRenderPref::MotionVectorBuffer,
		};
		constexpr int kCount = static_cast<int>(sizeof(kTextureNames) / sizeof(kTextureNames[0]));

		ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Pass I/O Texture Preview");
		if (ImGui::Combo("Texture", &m_selectedIndex, kTextureNames, kCount))
			m_selectedIndex = (m_selectedIndex < 0 || m_selectedIndex >= kCount) ? 0 : m_selectedIndex;

		auto rt = pipeline->GetRenderTargetForPreview(kTextureNames[m_selectedIndex], scene.get());
		if (rt && rt->GetSRVHandle() && rt->GetResource())
		{
			auto gpuHandle = g_Engine->GetImGuiSrvForTexture(rt->GetResource());
			if (gpuHandle.ptr != 0)
			{
				const float w = static_cast<float>(rt->GetWidth());
				const float h = static_cast<float>(rt->GetHeight());
				const float aspect = (h > 0) ? (w / h) : 1.0f;
				float displayW = w * 0.5f;
				float displayH = h * 0.5f;
				const float maxPreview = 320.0f;
				if (displayW > maxPreview || displayH > maxPreview)
				{
					if (aspect >= 1.0f)
					{
						displayW = maxPreview;
						displayH = maxPreview / aspect;
					}
					else
					{
						displayH = maxPreview;
						displayW = maxPreview * aspect;
					}
				}
				ImGui::Text("%s (%ux%u)", kTextureNames[m_selectedIndex], rt->GetWidth(), rt->GetHeight());
				ImGui::Image((ImTextureID)(uintptr_t)gpuHandle.ptr, ImVec2(displayW, displayH));
			}
			else
				ImGui::TextDisabled("(Failed to get ImGui SRV)");
		}
		else
			ImGui::TextDisabled("(Texture not available - check Deferred / RTGI / RTAO etc.)");

		ImGui::End();
	}

private:
	int m_selectedIndex = 0;
};
