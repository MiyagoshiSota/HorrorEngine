#include "App.h"

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "Components/Work/WorkManager.h"
#include "Modules/PublicConst//const_name_pref.h"
#include "Modules/PublicConst/const_path_pref.h"
#include "Renderer/Engine.h"
#include "Scene/SceneManager.h"

HINSTANCE g_hInst;
HWND g_hWnd = NULL;

std::shared_ptr<ISceneBase> g_Scene;
std::unique_ptr<SceneManager> g_SceneManager;
std::unique_ptr<ModelLoader> g_ModelLoader;
std::chrono::steady_clock::time_point g_lastFrameTime;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) 
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) return true;

	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wp,lp);
}

void init_window(const TCHAR* appName)
{
	g_hInst = GetModuleHandle(nullptr);
	if (g_hInst == nullptr) 
	{
		return;
	}

	//	Windowの設定
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(g_hInst, IDI_APPLICATION);
	wc.hCursor = LoadCursor(g_hInst, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_BACKGROUND);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = appName;
	wc.hIconSm = LoadIcon(g_hInst, IDI_APPLICATION);

	// ウィンドウクラスの設定
	RegisterClassEx(&wc);

	// ウィンドウサイズの登録
	RECT rect = {};
	rect.right = static_cast<LONG>(WINDOW_WIDTH);
	rect.bottom = static_cast<LONG>(WINDOW_HEIGHT);	
	
	// ウィンドウサイズを調整
	auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rect, style, FALSE);

	// ウィンドウの生成
	g_hWnd = CreateWindowEx
	 (
		0,
		appName,
		appName,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		nullptr,
		nullptr,
		g_hInst,
		nullptr
	);

	// ウィンドウを表示
	ShowWindow(g_hWnd, SW_SHOWNORMAL);

	// ウィンドウにフォーカスする
	SetFocus(g_hWnd);
}

void main_loop() {
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		// 現在の時刻を取得
		auto currentTime = std::chrono::steady_clock::now();

		// 前のフレームからの経過時間を計算
		std::chrono::duration<float> deltaTime = currentTime - g_lastFrameTime;

		// 次のフレームのために、現在の時刻を「最後の時刻」として保存
		g_lastFrameTime = currentTime;

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE == TRUE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else 
		{
			// SceneTypeに応じて更新処理を分岐
			if (g_scene_type == scene_type::play_mode)
			{
				g_Scene->Update(deltaTime.count());
			}
			else if (g_scene_type == scene_type::editor_mode)
			{
				g_Scene->EditorUpdate(deltaTime.count());
			}
			
			WorkManager::GetInstance().Update();

			g_Engine->BeginRender();
			g_Scene->Draw();
			g_Engine->EndRender();
			g_Engine->MoveToNextFrame();
		} 
	}
}

void  start_app(const TCHAR* appName, std::shared_ptr<ISceneBase> scene) {
	// Windowの初期化
	init_window(appName);

	// ModelLoaderの初期化
	g_ModelLoader = std::make_unique<ModelLoader>();
	if (!g_ModelLoader->init())
	{
		printf("モデルの読み込みに失敗しました。\n");
		return;
	}

	// 描画エンジンの初期化
	g_Engine = new Engine();
	if (!g_Engine->Init(g_hWnd,WINDOW_WIDTH,WINDOW_HEIGHT))
	{
		printf("描画エンジンの初期化に失敗しました。\n");
		return;
	}

	// SceneManagerの初期化
	g_SceneManager = std::make_unique<SceneManager>();
	
	// シーンの初期化
	g_Scene = scene;
if (!g_Scene->Init(const_path_pref::DefaultGameObjectPath))
	{
		printf("シーンの初期化に失敗しました。\n");
		return;
	}

	// メイン処理のループ
	main_loop();
}


void shutdown_app() {
	// シーンの終了処理
	g_Scene->shutdown();

	// 描画エンジンの終了処理
	if (g_Engine)
	{
		delete g_Engine;
		g_Engine = nullptr;
	}
	// ウィンドウの破棄
	if (g_hWnd)
	{
		DestroyWindow(g_hWnd);
		g_hWnd = nullptr;
	}
	// ウィンドウクラスの登録解除
	UnregisterClass(TEXT(const_name_pref::WindowName), g_hInst);
}