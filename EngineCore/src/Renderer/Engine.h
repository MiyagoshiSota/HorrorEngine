#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>

#include "Graphics/DescriptorHeap/SrvDescriptorHeap.h"
#include "GUI/IDrawWindow.h"
#include "Modules/ComPtr.h"

#pragma comment(lib,"d3d12.lib") // d3d12ライブラリをリンクする
#pragma comment(lib,"dxgi.lib") // dxgiライブラリをリンクする

class Engine
{
public:
	enum{ FRAME_BUFFER_COUNT = 2 }; // ダブルバッファリングするので2

public:
	~Engine()
	{
		Shutdown();
	};

	bool Init(HWND hwnd, UINT windowWidth, UINT windowHeight); // エンジン初期化
	void Shutdown(); // エンジン終了処理

	void BeginRender(); // 描画の開始処理
	void EndRender(); // 描画の終了処理
	void MoveToNextFrame();

	// 各種ゲッター
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHeap() { return m_pRtvHeap->GetCPUDescriptorHandleForHeapStart(); }
	std::shared_ptr<SrvDescriptorHeap> GetSrvHeap() { return m_SRVHeap; }
	// std::shared_ptr<CbvDescriptorHeap> GetCbvHeap() { return m_CBVHeap; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHeap() { return m_pDsvHeap->GetCPUDescriptorHandleForHeapStart(); }
	D3D12_VIEWPORT GetViewPort() { return m_Viewport; }
	D3D12_RECT GetScissorRect() { return m_Scissor; }

	// 各種アロケーター
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateRtvHandle();

public:
	ID3D12Device6* Device();
	ID3D12GraphicsCommandList* CommandList();
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
	ComPtr<ID3D12CommandAllocator> m_pAllocator[FRAME_BUFFER_COUNT] = { nullptr }; // コマンドアロケータ
	ComPtr<ID3D12GraphicsCommandList> m_pCommandList = nullptr; // コマンドリスト
	HANDLE m_fenceEvent = nullptr; // フェンスで使うイベント
	ComPtr<ID3D12Fence> m_pFence = nullptr; // フェンス
	UINT64 m_fenceValue[FRAME_BUFFER_COUNT]; // フェンスの値(ダブルバッファリング用に２)
	D3D12_VIEWPORT m_Viewport; // ビューポート
	D3D12_RECT m_Scissor; // シザー矩形

private: // 描画に使うオブジェクトとその生成関数たち
	bool CreateRenderTarget(); // レンダーターゲットを作成
	bool CreateShaderResourceViewHeap(); // SRVヒープを作成
	// bool CreateConstantBufferView(); // CBVを作成
	bool CreateDepthStencil(); // 深度ステンシルバッファを生成
	void DrawImGui(); // ImGuiの描画

	UINT m_RtvDescriptorSize = 0; // レンダーターゲットビューのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr; // レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pRenderTargets[FRAME_BUFFER_COUNT] = { nullptr }; // レンダーターゲット(ダブルバッファリングするので2個)

	UINT m_DsvDescriptorSize = 0; // 深度ステンシルのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr; // 深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12DescriptorHeap> m_ImGuiSrvHeap; // ImGui専用のSRVヒープ
	std::vector<std::shared_ptr<IDrawWindow>> m_drawWindows; // 描画するウィンドウのリスト
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr; // 深度ステンシルバッファ

	std::shared_ptr<SrvDescriptorHeap> m_SRVHeap; // SRVヒープ
	// std::shared_ptr<CbvDescriptorHeap> m_CBVHeap; // CBVヒープ

	UINT m_rtvHeapOffset; // RTVヒープのオフセット管理用

private:
	ComPtr < ID3D12Resource > m_currentRenderTarget = nullptr; // 現在のフレームのレンダーターゲットを一時的に保存しておく関数
	void WaitRender(); // 描画完了を待つ関数
};

extern Engine* g_Engine; // どこからでも参照したいのでグローバルにする
