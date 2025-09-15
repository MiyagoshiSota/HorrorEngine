#pragma once
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include "Modules/ComPtr.h"

#pragma comment(lib,"d3d12.lib") // d3d12ライブラリをリンクする
#pragma comment(lib,"dxgi.lib") // dxgiライブラリをリンクする

class Engine
{
public:
	enum{ FRAME_BUFFER_COUNT = 2 }; // ダブルバッファリングするので2

public:
	bool Init(HWND hwnd, UINT windowWidth, UINT windowHeight); // エンジン初期化

	void BeginRender(); // 描画の開始処理
	void EndRender(); // 描画の終了処理

public:
	ID3D12Device6* Device();
	ID3D12GraphicsCommandList* CommandList();
	UINT CurrentBackBufferIndex();

private:
	bool CreateDevice();
	bool CreateCommandQueue();
	bool CreateSwapChain();
	bool CreateCommandList();
	bool CreateFence();
	void CreateViewPort();
	void CreateScissorRect();

private:
	HWND m_hWnd;
	UINT m_FrameBufferWidth = 0;
	UINT m_FrameBufferHeight = 0;
	UINT m_CurrentBackBufferIndex = 0;

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
	bool CreateDepthStencil(); // 深度ステンシルバッファを生成
		
	UINT m_RtvDescriptorSize = 0; // レンダーターゲットビューのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pRtvHeap = nullptr; // レンダーターゲットのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pRenderTargets[FRAME_BUFFER_COUNT] = { nullptr }; // レンダーターゲット(ダブルバッファリングするので2個)

	UINT m_DsvDescriptorSize = 0; // 深度ステンシルのディスクリプタサイズ
	ComPtr<ID3D12DescriptorHeap> m_pDsvHeap = nullptr; // 深度ステンシルのディスクリプタヒープ
	ComPtr<ID3D12Resource> m_pDepthStencilBuffer = nullptr; // 深度ステンシルバッファ(こっちは1つでいい)

private:
	ID3D12Resource* m_currentRenderTarget = nullptr; // 現在のフレームのレンダーターゲットを一時的に保存しておく関数
	void WaitRender(); // 描画完了を待つ関数
};

extern Engine* g_Engine; // どこからでも参照したいのでグローバルにする
