#pragma once
#include <Windows.h>
#include "Scene/ISceneBase.h"
#include <memory>

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE
#include "Source/3DModel/Loader/ModelLoader.h"

const UINT kWindowWidth = 1980;
const UINT kWindowHeight = 1080;

enum class SceneType { EditorMode, PlayMode };

extern std::shared_ptr<ISceneBase> g_Scene;
extern std::unique_ptr<ModelLoader> g_ModelLoader;
extern std::chrono::steady_clock::time_point g_lastFrameTime;
#ifndef BUILD_STANDALONE
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // BUILD_STANDALONE

inline extern SceneType g_sceneType = SceneType::EditorMode;

void StartApp(const TCHAR* appName, std::shared_ptr<ISceneBase> scene);
void StartApp(const TCHAR* appName, std::shared_ptr<ISceneBase> scene, const std::string& scenePath);
void ShutdownApp();
inline void ChangeSceneType(SceneType type) { g_sceneType = type; };
