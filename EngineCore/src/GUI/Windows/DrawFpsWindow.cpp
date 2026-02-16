#include "DrawFpsWindow.h"
#include "Core/App.h"
#include "imgui.h"

void DrawFpsWindow::draw()
{
	if (!ImGui::Begin("Performance", &m_isVisible))
	{
		ImGui::End();
		return;
	}

	const float deltaSec = g_deltaTimeSeconds;
	const float msPerFrame = (deltaSec > 0.0f && deltaSec < 1.0f) ? (deltaSec * 1000.0f) : 0.0f;
	const float fps = (deltaSec > 0.0f && deltaSec < 1.0f) ? (1.0f / deltaSec) : 0.0f;

	ImGui::Text("FPS: %.1f", fps);
	ImGui::Text("ms/frame: %.2f", msPerFrame);

	ImGui::End();
}
