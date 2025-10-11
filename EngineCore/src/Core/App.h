#pragma once
#include <Windows.h>
#include "Scene/ISceneBase.h"
#include <memory>

#include "imgui.h"

const UINT WINDOW_WIDTH = 1920;
const UINT WINDOW_HEIGHT = 1080;

enum class scene_type { editor_mode, play_mode };

void start_app(const TCHAR* appName, std::shared_ptr<ISceneBase> scene);
void shutdown_app();

extern std::shared_ptr<ISceneBase> g_Scene;
extern std::chrono::steady_clock::time_point g_lastFrameTime;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline extern scene_type g_scene_type = scene_type::editor_mode;
