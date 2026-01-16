#pragma once
#include <Windows.h>
#include "Scene/ISceneBase.h"
#include <memory>

#include "imgui.h"
#include "Source/3DModel/Loader/ModelLoader.h"

const UINT WINDOW_WIDTH = 1700;
const UINT WINDOW_HEIGHT = 1000;

enum class scene_type { editor_mode, play_mode };

extern std::shared_ptr<ISceneBase> g_Scene;
extern std::unique_ptr<ModelLoader> g_ModelLoader;
extern std::chrono::steady_clock::time_point g_lastFrameTime;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

inline extern scene_type g_scene_type = scene_type::editor_mode;

void start_app(const TCHAR* appName, std::shared_ptr<ISceneBase> scene);
void shutdown_app();
inline void change_scene_type(scene_type type) { g_scene_type = type; };
