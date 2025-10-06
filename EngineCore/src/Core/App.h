#pragma once
#include <Windows.h>
#include "Scene/ISceneBase.h"
#include <memory>

const UINT WINDOW_WIDTH = 1920;
const UINT WINDOW_HEIGHT = 1080;
void start_app(const TCHAR* appName, std::shared_ptr<ISceneBase> scene);
extern std::shared_ptr<ISceneBase> g_Scene;

extern std::chrono::steady_clock::time_point g_lastFrameTime;