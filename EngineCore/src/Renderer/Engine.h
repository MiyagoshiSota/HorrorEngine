#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>

#include "Graphics/DescriptorHeap/DescriptorHeap.h"
#include "GUI/Core/IDrawWindow.h"
#include "GUI/Managers/LayoutPresetType.h"
#include "Modules/ComPtr.h"

#pragma comment(lib,"d3d12.lib") // d3d12ライブラリをリンクする
#pragma comment(lib,"dxgi.lib") // dxgiライブラリをリンクする

class Engine
{
public:
	enum{ kFrameBufferCount = 2 }; // ダブルバッファリングするので2

public:
	~Engine()
	{
		Shutdown();
	}

	void WaitForGPU();
	void CloseAndExecuteCommandList(); // コマンドリストを閉じて実行する（シーン切り替え時などに使用）

	bool Init(HWND hwnd, UINT windowWidth, UINT windowHeight); // エンジン初期化
	void Shutdown(); // エンジン終了処理

	void BeginRender(); // 描画の開始処理
	void EndRender(); // 描画の終了処理
	void MoveToNextFrame();

	// 各種ゲッター
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHeap() { return m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(); }
	std::shared_ptr<DescriptorHeap> GetDescriptorHeap() { return m_DescriptorHeap; }
	// std::shared_ptr<CbvDescriptorHeap> GetCbvHeap() { return m_CBVHeap; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHeap() { return m_pDsvHeap->GetCPUDescriptorHandleForHeapStart(); }
	D3D12_VIEWPORT GetViewPort() { return m_Viewport; }
	D3D12_RECT GetScissorRect() { return m_Scissor; }
	std::shared_ptr<Texture2D> GetTextureResource() { return m_TextureResource; }
	
	// ウィンドウリストへのアクセス
	const std::vector<std::shared_ptr<IDrawWindow>>& GetDrawWindows() const { return m_drawWindows; }
	std::vector<std::shared_ptr<IDrawWindow>>& GetDrawWindows() { return m_drawWindows; }
	
	// モードウィンドウへのアクセス
	std::shared_ptr<IDrawWindow> GetModeWindow() { return m_modeWindow; }
	
	// プリセット読み込みの予約（次フレームで読み込む）
	void SchedulePresetLoad(LayoutPresetType presetType) 
	{ 
		m_pendingPresetLoad = presetType; 
		m_hasPendingPresetLoad = true; 
	}

	// 各種アロケーター
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateRtvHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDsvHandle();

public:
	ID3D12Device6* Device();
	ID3D12GraphicsCommandList* CommandList();
	ID3D12CommandQueue* CommandQueue();
	UINT CurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRtvHandle() const;

private:
	bool CreateDevice();
	bool CreateCommandQueue();
	bool CreateSwapChain();
	bool CreateCommandList();
	bool CreateFence();
	void CreateViewPort();
	void CreateScissorRect();
	bool InitImGui();

private:
	HWND m_hWnd;
	UINT m_FrameBufferWidth = 0;
	UINT m_FrameBufferHeight = 0;
	UINT m_CurrentBackBufferIndex = 0;
	UINT mFrameCount = 0ull;

	ComPtr<ID3D12Device6> m_pDevice = nullptr; // デバイス
	ComPtr<ID3D12CommandQueue> m_pQueue = nullptr; // コマンドキュー
	ComPtr<IDXGISwapChain3> m_pSwapChain = nullptr; // スワップチェイン
	ComPtr<ID3D12CommandAllocator> m_pAllocator[kFrameBufferCount] = { nullptr }; // コマンドアロケータ
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList = nullptr; // コマンドリスト
	HANDLE m_fenceEvent = nullptr; // フェンスで使うイベント
	ComPtr<ID3D12Fence> m_pFence = nullptr; // フェンス
	UINT64 m_fenceValue[kFrameBufferCount]; // フェンスの値(ダブルバッファリング用に２)
	D3D12_VIEWPORT m_Viewport; // ビューポート
	D3D12_RECT m_Scissor; // シザー矩形

private: // 描画に使うオブジェクトとその生成関数たち
	bool CreateRenderTarget(); // レンダーターゲットを作成
	bool CreateDescriptorHeap(); // ディスクリプタヒープを作成
	bool CreateDepthStencil(); // 深度ステンシルバッファを生成
	void DrawImGui(); // ImGuiの描画

	UINT m_RtvDescriptorSize = 0; // レンダーターゲットビューのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr; // レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pRenderTargets[kFrameBufferCount] = { nullptr }; // レンダーターゲット(ダブルバッファリングするので2個)

	UINT m_DsvDescriptorSize = 0; // 深度ステンシルのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr; // 深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> m_ImGuiSrvHeap; // ImGui専用のSRVヒープ
	std::shared_ptr<IDrawWindow> m_mainMenuBar; // メインメニューバー（ドッキング不可能な固定UI）
	std::shared_ptr<IDrawWindow> m_modeWindow; // モードウィンドウ（Unity風の固定UI）
	std::vector<std::shared_ptr<IDrawWindow>> m_drawWindows; // 描画するウィンドウのリスト
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr; // 深度ステンシルバッファ

	std::shared_ptr<DescriptorHeap> m_DescriptorHeap; // SRVヒープ

	UINT m_rtvHeapOffset; // RTVヒープのオフセット管理用
	UINT m_dsvHeapOffset; // DSVヒープのオフセット管理用

	std::shared_ptr<Texture2D> m_TextureResource;
	bool m_isCommandListOpen = false; // コマンドリストが開いているかどうかを追跡
	bool m_presetApplied = false; // プリセットが適用済みかどうか
	LayoutPresetType m_pendingPresetLoad = LayoutPresetType::MakeMode; // 次フレームで読み込むプリセット
	bool m_hasPendingPresetLoad = false; // 次フレームでプリセットを読み込む必要があるか
private:
	ComPtr < ID3D12Resource > m_currentRenderTarget = nullptr; // 現在のフレームのレンダーターゲットを一時的に保存しておく関数
	void WaitRender(); // 描画完了を待つ関数
};

extern Engine* g_Engine; // どこからでも参照したいのでグローバルにする
