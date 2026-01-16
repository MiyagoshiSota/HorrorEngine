#include "Engine.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <stdio.h>
#include <Windows.h>
#include <DirectXTex.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "Scene/Default/Scene/DefaultScene.h"


#include "Graphics/DescriptorHeap/DescriptorHeap.h"
#include "GUI/DrawGameObjectWindow.h"
#include "GUI/DrawModeWindow.h"
#include "GUI/DrawPostProcessPresetWindow.h"
#include <GUI/DrawDayWindow.h>

#include "GUI/DrawModelsWindow.h"
#include "GUI/DrawTaskManagerWindow.h"
#include "GUI/DrawWorkManagerWindow.h"
#include "Texture/Texture2D.h"

Engine* g_Engine;

bool Engine::Init(HWND hwnd, UINT windowWidth, UINT windowHeight)
{
    m_FrameBufferWidth = windowWidth;
    m_FrameBufferHeight = windowHeight;
    m_hWnd = hwnd;

#if defined(_DEBUG)
    // デバッグレイヤーを有効化
    ID3D12Debug* debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        debugController->Release();
    }
#endif

    if (!CreateDevice()) {
        printf("デバイスの初期化に失敗");
        return false;
    }

    // CreateDevice()内、デバイス作成が成功した後に追加
#if defined(_DEBUG)
    ID3D12InfoQueue* pInfoQueue = nullptr;
    if (SUCCEEDED(m_pDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
    {
        // 重大なエラーが発生した場合、プログラムを停止させる
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

        // 特定のエラーメッセージを抑制する例（必要に応じて）
        // D3D12_MESSAGE_ID messageIds[] = { ... };
        // D3D12_INFO_QUEUE_FILTER filter = {};
        // filter.DenyList.NumIDs = _countof(messageIds);
        // filter.DenyList.pIDList = messageIds;
        // pInfoQueue->AddStorageFilterEntries(&filter);

        pInfoQueue->Release();
    }
#endif

    if (!CreateCommandQueue())
    {
        printf("コマンドキューの生成に失敗");
        return false;
    }

    if (!CreateSwapChain()) {
        printf("スワップチェインの作成に失敗");
        return false;
    }

    if (!CreateCommandList()) {
        printf("コマンドリストの作成に失敗");
        return false;
    }

    if (!CreateFence()) {
        printf("フェンスの作成に失敗");
        return false;
    }

    CreateViewPort();
    CreateScissorRect();

    if (!CreateDescriptorHeap())
    {
        printf("SRVHeapの作成に失敗");
        return false;
    }

    // if (!CreateConstantBufferView())
    // {
    //     printf("CBVHeapの作成に失敗");
    //     return false;
    // }

    if (!CreateRenderTarget()) {
        printf("レンダーターゲットの作成に失敗");
        return false;
    }

    if (!CreateDepthStencil()) {
        printf("デプスステンシルバッファの生成に失敗");
        return false;
    }

    if (!InitImGui())
    {
        printf("ImGuiの初期化に失敗");
		return false;
    }

	// TextureResourceの初期化
	m_TextureResource = std::make_shared<Texture2D>();

    printf("描画エンジンの初期化成功\n");
    return true;
}

void Engine::Shutdown()
{
	// GPUの処理が終わるまで待つ
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++) {
		MoveToNextFrame();
	}

	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	if (m_fenceEvent) {
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
	printf("描画エンジンの終了処理完了\n");
}

void Engine::BeginRender()
{
    // 現在のレンダーターゲットを更新
    m_currentRenderTarget = m_pRenderTargets[m_CurrentBackBufferIndex].Get();

    // コマンドを初期化してためる準備をする
    m_pAllocator[m_CurrentBackBufferIndex]->Reset();
    m_pCommandList->Reset(m_pAllocator[m_CurrentBackBufferIndex].Get(), nullptr);

    // ビューポートとシザー矩形を設定
    m_pCommandList->RSSetViewports(1, &m_Viewport);
    m_pCommandList->RSSetScissorRects(1, &m_Scissor);

    // 現在のフレームのレンダーターゲットビューのディスクリプタヒープの開始アドレスを取得
    auto currentRtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    currentRtvHandle.ptr += m_CurrentBackBufferIndex * m_RtvDescriptorSize;

    // 深度ステンシルのディスクリプタヒープの開始アドレス取得
    auto currentDsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

    // レンダーターゲットが使用可能になるまで待つ
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_currentRenderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_pCommandList->ResourceBarrier(1, &barrier);

    // レンダーターゲットを設定
    m_pCommandList->OMSetRenderTargets(1, &currentRtvHandle, FALSE, &currentDsvHandle);

    //// レンダーターゲットをクリア
    const float clearColor[] = { .5f,0.25f,0.25f,1.0f };
    m_pCommandList->ClearRenderTargetView(currentRtvHandle, clearColor, 0, nullptr);

    // 深度ステンシルビューをクリア
    m_pCommandList->ClearDepthStencilView(currentDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Engine::EndRender()
{
    // レンダーターゲットに書き込み終わるまで待つ
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_currentRenderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_pCommandList->ResourceBarrier(1, &barrier);

	// BackBufferを取得
    UINT frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();
    ID3D12Resource* backBuffer = m_pRenderTargets[frameIndex].Get();

    // BackBufferをPRESENT → RENDER_TARGET へ遷移
    D3D12_RESOURCE_BARRIER imGuibarrier = {};
    imGuibarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    imGuibarrier.Transition.pResource = backBuffer;
    imGuibarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    imGuibarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    imGuibarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_pCommandList->ResourceBarrier(1, &imGuibarrier);

    // Render target view 設定
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += frameIndex * m_RtvDescriptorSize;
    m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // ImGui 描画前の準備
    ID3D12DescriptorHeap* heaps[] = { m_ImGuiSrvHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // ImGuiの描画
    DrawImGui();

    // BackBufferをRENDER_TARGET → PRESENT に戻す
    imGuibarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    imGuibarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    m_pCommandList->ResourceBarrier(1, &imGuibarrier);

    // コマンドを完了して送信
    m_pCommandList->Close();
    ID3D12CommandList* cmdLists[] = { m_pCommandList.Get() };
    m_pQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // スワップチェインを切り替える
    m_pSwapChain->Present(1, 0);
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::AllocateRtvHandle()
{
    // ヒープの先頭ハンドルを取得
    auto handle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

    // 現在のオフセットを使ってアドレスを計算
    handle.ptr += m_rtvHeapOffset * m_RtvDescriptorSize;

    // 次の呼び出しのためにオフセットを1つ進める
    m_rtvHeapOffset++;

    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::AllocateDsvHandle()
{
	auto handle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += m_dsvHeapOffset * m_DsvDescriptorSize;
    
	m_dsvHeapOffset++;
	
	return handle;
}

ID3D12Device6* Engine::Device()
{
    return m_pDevice.Get();
}

ID3D12GraphicsCommandList* Engine::CommandList()
{
    return m_pCommandList.Get();
}

UINT Engine::CurrentBackBufferIndex()
{
    return m_CurrentBackBufferIndex;
}

bool Engine::CreateDevice()
{
    auto hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_pDevice.ReleaseAndGetAddressOf()));
    return SUCCEEDED(hr);
}

bool Engine::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    auto hr = m_pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(m_pQueue.ReleaseAndGetAddressOf()));

    return SUCCEEDED(hr);
}

bool Engine::CreateSwapChain()
{
    // DXGIファクトリーの作成
    IDXGIFactory4* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
    if (FAILED(hr)) {
        return false;
    }

    // スワップチェインの作成  
    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferDesc.Width = m_FrameBufferWidth;
    desc.BufferDesc.Height = m_FrameBufferHeight;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = FRAME_BUFFER_COUNT;
    desc.OutputWindow = m_hWnd;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // スワップチェインの作成
    IDXGISwapChain* pSwapChain = nullptr;
    hr = pFactory->CreateSwapChain(m_pQueue.Get(), &desc, &pSwapChain);
    if (FAILED(hr)) {
        pFactory->Release();
        return false;
    }

    // IDXGISwapChain3を取得
    hr = pSwapChain->QueryInterface(IID_PPV_ARGS(m_pSwapChain.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        pFactory->Release();
        pSwapChain->Release();
        return false;
    }

    // バックバッファ番号を取得
    m_CurrentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    pFactory->Release();
    pSwapChain->Release();
    return true;
}

bool Engine::CreateCommandList()
{
    // コマンドアロケータの作成
    HRESULT hr;
    for (size_t i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        hr = m_pDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(m_pAllocator[i].ReleaseAndGetAddressOf())
            );
    }

    if (FAILED(hr)) {
        return false;
    }

    // コマンドリストの作成
    hr = m_pDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_pAllocator[m_CurrentBackBufferIndex].Get(),
        nullptr,
        IID_PPV_ARGS(&m_pCommandList)
    );

    if (FAILED(hr)) {
        return false;
    }

    // コマンドリストは開かれている状態で作成されるのでいったん閉じる
    m_pCommandList->Close();

    return true;
}

bool Engine::CreateFence()
{
    for (auto i = 0u; i < FRAME_BUFFER_COUNT; i++)
    {
        m_fenceValue[i] = 0;
    }

    auto hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.ReleaseAndGetAddressOf()));
    if (FAILED(hr)) {
        return false;
    }

    m_fenceValue[m_CurrentBackBufferIndex]++;

    // 同期を行う時のイベントハンドラを作成する
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    return m_fenceEvent != nullptr;
}

void Engine::CreateViewPort()
{
    m_Viewport.TopLeftX = 0;
    m_Viewport.TopLeftY = 0;
    m_Viewport.Width = static_cast<float>(m_FrameBufferWidth);
    m_Viewport.Height = static_cast<float>(m_FrameBufferHeight);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
}

void Engine::CreateScissorRect()
{
    m_Scissor.left = 0;
    m_Scissor.right = m_FrameBufferWidth;
    m_Scissor.top = 0;
    m_Scissor.bottom = m_FrameBufferHeight;
}

bool Engine::InitImGui()
{
    // 1. SRVヒープ作成
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = 64; // ← フォント＋テクスチャ用に余裕を持たせる
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    HRESULT hr = m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_ImGuiSrvHeap));
    if (FAILED(hr)) return false;

    // 2. ImGuiコンテキスト作成
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // 3. バックエンド初期化 (新API)
    ImGui_ImplWin32_Init(m_hWnd);

    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = m_pDevice.Get();
    init_info.CommandQueue = m_pQueue.Get();
    init_info.NumFramesInFlight = FRAME_BUFFER_COUNT;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    init_info.SrvDescriptorHeap = m_ImGuiSrvHeap.Get();
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
    {
        auto heap = info->SrvDescriptorHeap;
        auto device = info->Device;
        UINT size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        *out_cpu_handle = heap->GetCPUDescriptorHandleForHeapStart();
        *out_gpu_handle = heap->GetGPUDescriptorHandleForHeapStart();
    };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {};

    ImGui_ImplDX12_Init(&init_info);

    // 4. フォント読み込み（省略可能）
    io.Fonts->AddFontDefault();

    // 描画するウィンドウのリストに追加
    m_drawWindows.push_back(std::make_shared<DrawModeWindow>());
    m_drawWindows.push_back(std::make_shared<DrawGameObjectWindow>());
    m_drawWindows.push_back(std::make_shared<DrawPostProcessPresetWindow>());
    m_drawWindows.push_back(std::make_shared<DrawDayWindow>());
    m_drawWindows.push_back(std::make_shared<DrawModelsWindow>());
    m_drawWindows.push_back(std::make_shared<DrawWorkManagerWindow>());
    
    return true;
}

bool Engine::CreateRenderTarget()
{
    // RTV用のディスクリプタヒープを作成する
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};

    // HACK:格納できるHeap数注意
    desc.NumDescriptors = FRAME_BUFFER_COUNT + 10;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    auto hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_pRtvHeap.ReleaseAndGetAddressOf()));
    if (FAILED(hr))
    {
        return false;
    }

    // ディスクリプタのサイズを取得
    m_RtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < FRAME_BUFFER_COUNT; i++)
    {
        m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].ReleaseAndGetAddressOf()));
        m_pDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_RtvDescriptorSize;
    }

    m_rtvHeapOffset = FRAME_BUFFER_COUNT;

    return true;
}

bool Engine::CreateDescriptorHeap()
{
    m_DescriptorHeap = std::make_shared<DescriptorHeap>(2048);
    return true;
}

// bool Engine::CreateConstantBufferView()
// {
//     m_CBVHeap = std::make_shared<CbvDescriptorHeap>(2048);
//     return true;
// }

bool Engine::CreateDepthStencil()
{
    // DSV用のディスクリプタヒープを作成する
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

    heapDesc.NumDescriptors = FRAME_BUFFER_COUNT + 30;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    auto hr = m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pDsvHeap));
    if (FAILED(hr)) {
        return false;
    }

    // ディスクリプタのサイズを取得
    m_DsvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_CLEAR_VALUE dsvClearValue;
    dsvClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    dsvClearValue.DepthStencil.Depth = 1.0f;
    dsvClearValue.DepthStencil.Stencil = 0;

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC resourceDesc(
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0,
        m_FrameBufferWidth,
        m_FrameBufferHeight,
        1,
        1,
        DXGI_FORMAT_D32_FLOAT,
        1,
        0,
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
    hr = m_pDevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &dsvClearValue,
        IID_PPV_ARGS(m_pDepthStencilBuffer.ReleaseAndGetAddressOf())
    );

    if (FAILED(hr))
    {
        return false;
    }

    // ディスクリプタを作成
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();

    m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), nullptr, dsvHandle);

	m_dsvHeapOffset = 1;

    return true;
}

void Engine::DrawImGui()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // 登録されているウィンドウを順番に描画
    for (const auto& window : m_drawWindows)
    {
        window->draw();
    }
    
    ImGui::Render();

    // ImGui の描画実行
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pCommandList.Get());
}

void Engine::MoveToNextFrame()
{
    // 1. 今まさにGPUに投げた描画処理の完了を予約する
    const UINT64 currentFenceValue = m_fenceValue[m_CurrentBackBufferIndex];
    m_pQueue->Signal(m_pFence.Get(), currentFenceValue);

    // 2. 次のフレームで使うバックバッファのインデックスを取得
    m_CurrentBackBufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    // 3. GPUが、"次のフレームで使うリソース"をまだ使い終わっているかチェック
    if (m_pFence->GetCompletedValue() < m_fenceValue[m_CurrentBackBufferIndex])
    {
        // 4. もし終わっていなければ、完了するまで待機する
        m_pFence->SetEventOnCompletion(m_fenceValue[m_CurrentBackBufferIndex], m_fenceEvent);
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // 5. 次回、このバックバッファの完了をチェックするためのフェンス値を設定する
    m_fenceValue[m_CurrentBackBufferIndex] = currentFenceValue + 1;
}

D3D12_CPU_DESCRIPTOR_HANDLE Engine::GetCurrentRtvHandle() const
{
    auto rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += m_CurrentBackBufferIndex * m_RtvDescriptorSize;
    return rtvHandle;
}

void Engine::WaitForGPU()
{
    // コマンドキューに、現在のフェンス値を完了目標として設定（シグナル）
    const UINT64 fenceValue = m_fenceValue[m_CurrentBackBufferIndex];
    m_pQueue->Signal(m_pFence.Get(), fenceValue);

    // GPUが目標のフェンス値に到達しているか確認
    if (m_pFence->GetCompletedValue() < fenceValue)
    {
        // 到達していなければ、完了時にイベントを発行するようGPUに指示
        m_pFence->SetEventOnCompletion(fenceValue, m_fenceEvent);
        
        // イベントが発行される（GPUの処理が終わる）まで、CPUをここで待機させる
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    // 次の同期のために、フェンス値をインクリメント
    m_fenceValue[m_CurrentBackBufferIndex]++;
}
