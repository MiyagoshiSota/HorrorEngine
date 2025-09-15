#include "App.h"
#include "Renderer/Engine.h"
#include "Scene/Scene.h"

HINSTANCE g_hInst;
HWND g_hWnd = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) 
{
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

void MainLoop() {
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE == TRUE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			g_Scene->Update();
			g_Engine->BeginRender();
			g_Scene->Draw();
			g_Engine->EndRender();
		}
	}
}

void StartApp(const TCHAR* appName) {
	// Windowの初期化
	InitWindow(appName);

	// 描画エンジンの初期化
	g_Engine = new Engine();
	if (!g_Engine->Init(g_hWnd,WINDOW_WIDTH,WINDOW_HEIGHT))
	{
		return;
	}

	// シーンの初期化
	g_Scene = new Scene();
	if (!g_Scene->Init())
	{
		return;
	}

	// メイン処理のループ
	MainLoop();
}
