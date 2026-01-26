#include "App.h"

#ifndef BUILD_STANDALONE
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#endif // BUILD_STANDALONE
#include "Components/Work/WorkManager.h"
#include "Input/InputDevice.h"
#include "Modules/PublicConst/ConstNamePref.h"
#include "Modules/PublicConst/ConstPathPref.h"
#include "Renderer/Engine.h"
#include "Scene/SceneManager.h"

HINSTANCE g_hInst;
HWND g_hWnd = NULL;

std::shared_ptr<ISceneBase> g_Scene;
std::unique_ptr<SceneManager> g_SceneManager;
std::unique_ptr<ModelLoader> g_ModelLoader;
std::chrono::steady_clock::time_point g_lastFrameTime;

// WM_KEYUP遅延測定用の統計情報
struct KeyUpDelayStats
{
    float maxDelay = 0.0f;      // 最大遅延（ミリ秒）
    float totalDelay = 0.0f;    // 累積遅延（ミリ秒）
    int count = 0;              // 処理されたWM_KEYUPの数
    float lastDelay = 0.0f;     // 最後の遅延（ミリ秒）
} g_keyUpDelayStats;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) 
{
#ifndef BUILD_STANDALONE
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) return true;
#endif // BUILD_STANDALONE

	// 入力処理
	InputDevice::GetInstance().ProcessMessage(msg, wp, lp);
	
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wp,lp);
}

void InitWindow(const TCHAR* appName)
{
	g_hInst = GetModuleHandle(nullptr);
	if (g_hInst == nullptr) 
	{
		return;
	}

	// Windowの設定
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
	rect.right = static_cast<LONG>(kWindowWidth);
	rect.bottom = static_cast<LONG>(kWindowHeight);	
	
	// ウィンドウサイズを調整
	const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
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

void MainLoop() {
	printf("[MainLoop] 開始\n");
	MSG msg = {};
	int frameCount = 0;

	try {
		while (WM_QUIT != msg.message)
		{
			frameCount++;
			if (frameCount == 1)
			{
				printf("[MainLoop] 最初のフレーム開始\n");
			}

			// 現在の時刻を取得
			const auto currentTime = std::chrono::steady_clock::now();

			// 前のフレームからの経過時間を計算
			std::chrono::duration<float> deltaTime = currentTime - g_lastFrameTime;

			// 次のフレームのために、現在の時刻を「最後の時刻」として保存
			g_lastFrameTime = currentTime;

			if (frameCount == 1)
			{
				printf("[MainLoop] メッセージ処理を開始\n");
			}

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					printf("[MainLoop] WM_QUIT受信\n");
					break;
				}
				
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (frameCount == 1)
			{
				printf("[MainLoop] メッセージ処理完了\n");
			}

			// 経過時間を秒単位で取得
			const float deltaTimeFloat = deltaTime.count();

			if (frameCount == 1)
			{
				printf("[MainLoop] 入力デバイスの更新を開始\n");
			}

			// 入力デバイスの更新
			InputDevice::GetInstance().Update(deltaTimeFloat);
			
			if (frameCount == 1)
			{
				printf("[MainLoop] 入力デバイスの更新完了\n");
				printf("[MainLoop] SceneType: %s\n", g_sceneType == SceneType::PlayMode ? "PlayMode" : "EditorMode");
			}

			// SceneTypeに応じて更新処理を分岐
			if (g_sceneType == SceneType::PlayMode)
			{
				if (frameCount == 1)
				{
					printf("[MainLoop] Scene->Update()を呼び出し\n");
				}
				g_Scene->Update(deltaTimeFloat);
				if (frameCount == 1)
				{
					printf("[MainLoop] WorkManager->Update()を呼び出し\n");
				}
				WorkManager::GetInstance().Update(deltaTimeFloat);
			}
			else if (g_sceneType == SceneType::EditorMode)
			{
				if (frameCount == 1)
				{
					printf("[MainLoop] Scene->EditorUpdate()を呼び出し\n");
				}
				g_Scene->EditorUpdate(deltaTime.count());
			}

			if (frameCount == 1)
			{
				printf("[MainLoop] BeginRender()を呼び出し\n");
			}
			g_Engine->BeginRender();
			
			if (frameCount == 1)
			{
				printf("[MainLoop] Scene->Draw()を呼び出し\n");
			}
			g_Scene->Draw();
			
			if (frameCount == 1)
			{
				printf("[MainLoop] EndRender()を呼び出し\n");
			}
			g_Engine->EndRender();
			
			if (frameCount == 1)
			{
				printf("[MainLoop] MoveToNextFrame()を呼び出し\n");
			}
			g_Engine->MoveToNextFrame();
			
			if (frameCount == 1)
			{
				printf("[MainLoop] 最初のフレーム完了\n");
			}
		}
		printf("[MainLoop] ループ終了 (frameCount=%d)\n", frameCount);
	}
	catch (const std::exception& e)
	{
		printf("[MainLoop] 例外発生 (frameCount=%d): %s\n", frameCount, e.what());
		throw;
	}
	catch (...)
	{
		printf("[MainLoop] 不明な例外が発生しました (frameCount=%d)\n", frameCount);
		throw;
	}
}

void  StartApp(const TCHAR* appName, std::shared_ptr<ISceneBase> scene) {
	StartApp(appName, scene, ConstPathPref::kDefaultGameObjectPath);
}

void  StartApp(const TCHAR* appName, std::shared_ptr<ISceneBase> scene, const std::string& scenePath) {
	printf("[StartApp] 開始: scenePath=%s\n", scenePath.c_str());
	
	try {
		// Windowの初期化
		printf("[StartApp] Windowの初期化を開始\n");
		InitWindow(appName);
		printf("[StartApp] Windowの初期化完了\n");

		// 描画エンジンの初期化
		printf("[StartApp] 描画エンジンの初期化を開始\n");
		g_Engine = new Engine();
		if (!g_Engine->Init(g_hWnd,kWindowWidth,kWindowHeight))
		{
			printf("[StartApp] 描画エンジンの初期化に失敗しました。\n");
			return;
		}
		printf("[StartApp] 描画エンジンの初期化完了\n");

		// ModelLoaderの初期化
		printf("[StartApp] ModelLoaderの初期化を開始\n");
		g_ModelLoader = std::make_unique<ModelLoader>();
		if (!g_ModelLoader->Init())
		{
			printf("[StartApp] モデルの読み込みに失敗しました。\n");
			return;
		}
		printf("[StartApp] ModelLoaderの初期化完了\n");

		// SceneManagerの初期化
		printf("[StartApp] SceneManagerの初期化を開始\n");
		g_SceneManager = std::make_unique<SceneManager>();
		printf("[StartApp] SceneManagerの初期化完了\n");
		
		// シーンの初期化
		printf("[StartApp] シーンの初期化を開始\n");
		g_Scene = scene;
		if (!g_Scene->Init(scenePath))
		{
			printf("[StartApp] シーンの初期化に失敗しました。\n");
			return;
		}
		printf("[StartApp] シーンの初期化完了\n");

		// SceneTypeの確認
		printf("[StartApp] SceneType: %s\n", g_sceneType == SceneType::PlayMode ? "PlayMode" : "EditorMode");

		// メイン処理のループ
		printf("[StartApp] MainLoopを開始\n");
		MainLoop();
		printf("[StartApp] MainLoop終了\n");
	}
	catch (const std::exception& e)
	{
		printf("[StartApp] 例外発生: %s\n", e.what());
		throw;
	}
	catch (...)
	{
		printf("[StartApp] 不明な例外が発生しました\n");
		throw;
	}
}


void ShutdownApp() {
	// シーンの終了処理
	g_Scene->Shutdown();

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
	UnregisterClass(TEXT(ConstNamePref::WindowName), g_hInst);
}