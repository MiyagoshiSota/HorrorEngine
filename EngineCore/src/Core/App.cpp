#include "App.h"

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
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
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wp, lp)) return true;

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
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		// 現在の時刻を取得
		const auto currentTime = std::chrono::steady_clock::now();

		// 前のフレームからの経過時間を計算
		std::chrono::duration<float> deltaTime = currentTime - g_lastFrameTime;

		// 次のフレームのために、現在の時刻を「最後の時刻」として保存
		g_lastFrameTime = currentTime;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
			
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			
		}

		// 経過時間を秒単位で取得
		const float deltaTimeFloat = deltaTime.count();

		// 入力デバイスの更新
		InputDevice::GetInstance().Update(deltaTimeFloat);
		
		// SceneTypeに応じて更新処理を分岐
		if (g_sceneType == SceneType::PlayMode)
		{
			g_Scene->Update(deltaTimeFloat);
			WorkManager::GetInstance().Update(deltaTimeFloat);
		}
		else if (g_sceneType == SceneType::EditorMode)
		{
			g_Scene->EditorUpdate(deltaTime.count());
		}

		g_Engine->BeginRender();
		g_Scene->Draw();
		g_Engine->EndRender();
		g_Engine->MoveToNextFrame();
	}
}

void  StartApp(const TCHAR* appName, std::shared_ptr<ISceneBase> scene) {
	// Windowの初期化
	InitWindow(appName);

	// 描画エンジンの初期化
	g_Engine = new Engine();
	if (!g_Engine->Init(g_hWnd,kWindowWidth,kWindowHeight))
	{
		printf("描画エンジンの初期化に失敗しました。\n");
		return;
	}

	// ModelLoaderの初期化
	g_ModelLoader = std::make_unique<ModelLoader>();
	if (!g_ModelLoader->Init())
	{
		printf("モデルの読み込みに失敗しました。\n");
		return;
	}

	// SceneManagerの初期化
	g_SceneManager = std::make_unique<SceneManager>();
	
	// シーンの初期化
	g_Scene = scene;
if (!g_Scene->Init(ConstPathPref::kDefaultGameObjectPath))
	{
		printf("シーンの初期化に失敗しました。\n");
		return;
	}

	// メイン処理のループ
	MainLoop();
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