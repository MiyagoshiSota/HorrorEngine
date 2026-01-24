#include "Texture2D.h"
#include <DirectXTex.h>
#include <vector>
#include <cassert>
#include <d3dx12.h>
#include <chrono>
#include <iostream>
#include <Windows.h>

#include "Renderer/Engine.h"
#include "Modules/DxHelper.h"

// ライブラリのリンク
#pragma comment(lib, "DirectXTex.lib")

using namespace DirectX;

// --- ヘルパー関数 ---

// std::string -> std::wstring 変換
std::wstring GetWideString(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// 拡張子の取得
std::wstring FileExtension(const std::wstring& path)
{
    auto idx = path.rfind(L'.');
    if (idx == std::wstring::npos) return L"";
    return path.substr(idx + 1);
}

// --- コンストラクタ・デストラクタ ---

Texture2D::Texture2D()
    : m_width(0), m_height(0)
{
}

Texture2D::~Texture2D()
{
}

// --- ファクトリーメソッド (外部から呼ばれる) ---

std::shared_ptr<Texture2D> Texture2D::Load(const std::wstring& path)
{
    // インスタンス生成
    auto tex = std::make_shared<Texture2D>();

    // 内部ロード処理を実行
    if (!tex->InternalLoad(path))
    {
        // 失敗したらnullptrを返す
        return nullptr;
    }
    return tex;
}

std::shared_ptr<Texture2D> Texture2D::CreateWhiteTexture()
{
    auto tex = std::make_shared<Texture2D>();

    // 1x1ピクセルの白データ (RGBA = 0xFF, 0xFF, 0xFF, 0xFF)
    uint32_t whitePixel = 0x00000000;
	uint32_t blackPixel = 0x000000FF;

    // 内部メソッドでDX12リソースを作成
    bool success = tex->InternalCreateFromData(
        reinterpret_cast<const uint8_t*>(&whitePixel),
        sizeof(uint32_t),
        1,
        1
    );

    if (!success)
    {
        printf("白テクスチャの作成に失敗\n");
        return nullptr;
    }

    return tex;
}

// --- 内部実装 ---

bool Texture2D::InternalLoad(const std::wstring& path)
{
    // 全体の読み込み開始時刻を記録
    const auto totalStartTime = std::chrono::high_resolution_clock::now();

    m_path = path;

    // === 1. ファイルI/O + デコード時間の計測 ===
    const auto fileIoStartTime = std::chrono::high_resolution_clock::now();

    // DirectXTexを使って画像をロード
    TexMetadata meta = {};
    ScratchImage scratch = {};
    std::wstring ext = FileExtension(path);
    HRESULT hr = E_FAIL;

    // 拡張子による分岐 (小文字化比較などが望ましいですが簡易実装)
    if (ext == L"tga" || ext == L"TGA")
    {
        hr = LoadFromTGAFile(path.c_str(), &meta, scratch);
    }
    else
    {
        // png, jpg, bmp などはWIC
        hr = LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, &meta, scratch);
    }

    const auto fileIoEndTime = std::chrono::high_resolution_clock::now();
    const auto fileIoDuration = std::chrono::duration_cast<std::chrono::microseconds>(fileIoEndTime - fileIoStartTime);
    const double fileIoMs = fileIoDuration.count() / 1000.0;

    try
    {
        ThrowIfFailed(hr);
    }
    catch (const std::exception& e)
    {
        printf("画像ロード失敗: %ls - %s\n", path.c_str(), e.what());
        return false;
    }

    // メタデータの保存
    m_width = static_cast<uint32_t>(meta.width);
    m_height = static_cast<uint32_t>(meta.height);

    const Image* img = scratch.GetImage(0, 0, 0);

    // === 2. GPUリソース作成時間の計測 ===
    const auto resourceCreateStartTime = std::chrono::high_resolution_clock::now();

    // リソースを確保 (フォーマットは画像に合わせる)
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        meta.format,
        meta.width,
        meta.height,
        static_cast<UINT16>(meta.arraySize),
        static_cast<UINT16>(meta.mipLevels)
    );

    // Phase 1: D3D12_HEAP_TYPE_DEFAULTでGPU専用メモリに作成（COPY_DEST状態）
    auto defaultHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
            &defaultHeapProp,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, // コピー先として使用
            nullptr,
            IID_PPV_ARGS(&m_resource)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    const auto resourceCreateEndTime = std::chrono::high_resolution_clock::now();
    const auto resourceCreateDuration = std::chrono::duration_cast<std::chrono::microseconds>(resourceCreateEndTime - resourceCreateStartTime);
    const double resourceCreateMs = resourceCreateDuration.count() / 1000.0;

    // === 3. レイアウト計算時間の計測 ===
    const auto layoutCalcStartTime = std::chrono::high_resolution_clock::now();

    // GetCopyableFootprintsでレイアウト計算
    UINT64 uploadBufferSize = 0;
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};

    g_Engine->Device()->GetCopyableFootprints(
        &resDesc,
        0,
        1,
        0,
        &layout,
        &numRows,
        &rowSizeInBytes,
        &uploadBufferSize
    );

    const auto layoutCalcEndTime = std::chrono::high_resolution_clock::now();
    const auto layoutCalcDuration = std::chrono::duration_cast<std::chrono::microseconds>(layoutCalcEndTime - layoutCalcStartTime);
    const double layoutCalcMs = layoutCalcDuration.count() / 1000.0;

    // === 4. ステージングバッファ作成時間の計測 ===
    const auto stagingCreateStartTime = std::chrono::high_resolution_clock::now();

    // UPLOADヒープでステージングバッファを作成
    auto uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
    try
    {
        ThrowIfFailed(g_Engine->Device()->CreateCommittedResource(
            &uploadHeapProp,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer)
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    const auto stagingCreateEndTime = std::chrono::high_resolution_clock::now();
    const auto stagingCreateDuration = std::chrono::duration_cast<std::chrono::microseconds>(stagingCreateEndTime - stagingCreateStartTime);
    const double stagingCreateMs = stagingCreateDuration.count() / 1000.0;

    // === 5. CPU書き込み時間の計測 ===
    const auto cpuCopyStartTime = std::chrono::high_resolution_clock::now();

    // Map/memcpy/UnmapでCPUからステージングバッファにデータコピー
    uint8_t* pData = nullptr;
    try
    {
        D3D12_RANGE readRange = { 0, 0 }; // CPUからは読み取らない
        ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));

        // アライメントを考慮しながらデータをコピー
        const uint8_t* pSrcData = img->pixels;
        const UINT srcRowPitch = static_cast<UINT>(img->rowPitch);
        const UINT dstRowPitch = static_cast<UINT>(layout.Footprint.RowPitch);
        const UINT rowSize = static_cast<UINT>(rowSizeInBytes);

        for (UINT row = 0; row < numRows; ++row)
        {
            memcpy(pData + layout.Offset + row * layout.Footprint.RowPitch,
                   pSrcData + row * srcRowPitch,
                   rowSize);
        }

        D3D12_RANGE writeRange = { 0, static_cast<SIZE_T>(uploadBufferSize) };
        uploadBuffer->Unmap(0, &writeRange);
    }
    catch (const std::exception&)
    {
        return false;
    }

    const auto cpuCopyEndTime = std::chrono::high_resolution_clock::now();
    const auto cpuCopyDuration = std::chrono::duration_cast<std::chrono::microseconds>(cpuCopyEndTime - cpuCopyStartTime);
    const double cpuCopyMs = cpuCopyDuration.count() / 1000.0;

    // === 6. コマンドリスト作成時間の計測 ===
    const auto cmdListCreateStartTime = std::chrono::high_resolution_clock::now();

    // コマンドリストでCopyTextureRegionを発行
    // 専用のコマンドリストを作成（テクスチャロード用）
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    UINT64 fenceValue = 1;

    try
    {
        // コマンドアロケータ作成
        ThrowIfFailed(g_Engine->Device()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&commandAllocator)
        ));

        // コマンドリスト作成
        ThrowIfFailed(g_Engine->Device()->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            commandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&commandList)
        ));

        // フェンス作成
        ThrowIfFailed(g_Engine->Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    }
    catch (const std::exception&)
    {
        if (fenceEvent) CloseHandle(fenceEvent);
        return false;
    }

    const auto cmdListCreateEndTime = std::chrono::high_resolution_clock::now();
    const auto cmdListCreateDuration = std::chrono::duration_cast<std::chrono::microseconds>(cmdListCreateEndTime - cmdListCreateStartTime);
    const double cmdListCreateMs = cmdListCreateDuration.count() / 1000.0;

    // === 7. コマンド発行時間の計測 ===
    const auto cmdIssueStartTime = std::chrono::high_resolution_clock::now();
    
    // GPU転送開始時刻を記録するための変数
    std::chrono::high_resolution_clock::time_point gpuTransferStartTime;

    try
    {
        // コピーコマンドを発行
        CD3DX12_TEXTURE_COPY_LOCATION dst(m_resource.Get(), 0);
        CD3DX12_TEXTURE_COPY_LOCATION src(uploadBuffer.Get(), layout);
        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // リソースバリア: COPY_DEST → PIXEL_SHADER_RESOURCE
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_resource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
        commandList->ResourceBarrier(1, &barrier);

        // コマンドリストを閉じて実行
        ThrowIfFailed(commandList->Close());

        ID3D12CommandList* cmdLists[] = { commandList.Get() };
        
        // ExecuteCommandListsの直前に時刻を記録（GPU転送開始時刻）
        gpuTransferStartTime = std::chrono::high_resolution_clock::now();
        g_Engine->CommandQueue()->ExecuteCommandLists(1, cmdLists);
    }
    catch (const std::exception&)
    {
        if (fenceEvent) CloseHandle(fenceEvent);
        return false;
    }

    const auto cmdIssueEndTime = std::chrono::high_resolution_clock::now();
    const auto cmdIssueDuration = std::chrono::duration_cast<std::chrono::microseconds>(cmdIssueEndTime - cmdIssueStartTime);
    const double cmdIssueMs = cmdIssueDuration.count() / 1000.0;

    // === 8. GPUコピー（PCIe転送）とSync Overheadの計測 ===
    double syncOverheadMs = 0.0;
    double gpuCopyMs = 0.0;

    try
    {
        // フェンスで転送完了を待機（Signalを呼び出す）
        const auto signalBeforeTime = std::chrono::high_resolution_clock::now();
        ThrowIfFailed(g_Engine->CommandQueue()->Signal(fence.Get(), fenceValue));
        const auto signalAfterTime = std::chrono::high_resolution_clock::now();
        
        // Signal直後にフェンス完了をチェック
        const bool alreadyCompleted = (fence->GetCompletedValue() >= fenceValue);
        
        if (alreadyCompleted)
        {
            // Signal直後に完了している場合、転送は既に完了している
            // GPU Copy時間はExecuteCommandListsからSignalまでの時間
            const auto gpuCopyDuration = std::chrono::duration_cast<std::chrono::microseconds>(signalAfterTime - gpuTransferStartTime);
            gpuCopyMs = gpuCopyDuration.count() / 1000.0;
            syncOverheadMs = 0.0;
        }
        else
        {
            // フェンス完了を待機（Sync Overhead）
            // SetEventOnCompletionのセットアップ（この時間はSync Overheadに含める）
            ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
            
            // フェンス完了を待機
            WaitForSingleObject(fenceEvent, INFINITE);
            const auto waitEndTime = std::chrono::high_resolution_clock::now();
            
            // Sync Overhead = Signal呼び出し後からフェンス完了までの時間
            // これには、SetEventOnCompletionのセットアップ時間とWaitForSingleObjectの待機時間が含まれる
            const auto syncDuration = std::chrono::duration_cast<std::chrono::microseconds>(waitEndTime - signalAfterTime);
            syncOverheadMs = syncDuration.count() / 1000.0;
            
            // GPU Copy = ExecuteCommandListsから実際のGPU転送完了までの時間
            // 実際のGPU転送完了時刻はフェンス完了時刻（waitEndTime）に最も近い
            // しかし、Signal呼び出しからフェンス完了までの時間は同期オーバーヘッドなので、
            // GPU Copy = ExecuteCommandListsからフェンス完了までの時間 - Sync Overhead
            // つまり、ExecuteCommandListsからSignal呼び出しまでの時間が実際のGPU転送時間に最も近い
            const auto gpuCopyDuration = std::chrono::duration_cast<std::chrono::microseconds>(signalAfterTime - gpuTransferStartTime);
            gpuCopyMs = gpuCopyDuration.count() / 1000.0;
        }
    }
    catch (const std::exception&)
    {
        if (fenceEvent) CloseHandle(fenceEvent);
        return false;
    }

    CloseHandle(fenceEvent);

    // データ転送時間（CPUコピー + GPUコピー + Sync Overhead）として記録
    const double transferMs = cpuCopyMs + gpuCopyMs + syncOverheadMs;

    // === 4. その他処理時間の計測 ===
    const auto otherStartTime = std::chrono::high_resolution_clock::now();

    // SRV設定の作成
    m_srvDesc.Format = resDesc.Format;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    m_srvDesc.Texture2D.MipLevels = resDesc.MipLevels;
    // m_srvDesc.Texture2D.MostDetailedMip = 0;
    // m_srvDesc.Texture2D.PlaneSlice = 0;
    // m_srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    const auto otherEndTime = std::chrono::high_resolution_clock::now();
    const auto otherDuration = std::chrono::duration_cast<std::chrono::microseconds>(otherEndTime - otherStartTime);
    const double otherMs = otherDuration.count() / 1000.0;

    // === 全体時間の計測 ===
    const auto totalEndTime = std::chrono::high_resolution_clock::now();
    const auto totalDuration = std::chrono::duration_cast<std::chrono::microseconds>(totalEndTime - totalStartTime);
    const double totalMs = totalDuration.count() / 1000.0;

    // === 詳細ログの出力 ===
    std::wcout << L"[Texture Load] " << path << L" - " << m_width << L"x" << m_height << std::endl;
    std::wcout << L"  File I/O + Decode: " << fileIoMs << L" ms" << std::endl;
    std::wcout << L"  GPU Resource Creation: " << resourceCreateMs << L" ms" << std::endl;
    std::wcout << L"  Layout Calculation: " << layoutCalcMs << L" ms" << std::endl;
    std::wcout << L"  Staging Buffer Creation: " << stagingCreateMs << L" ms" << std::endl;
    std::wcout << L"  CPU Copy: " << cpuCopyMs << L" ms" << std::endl;
    std::wcout << L"  GPU Copy: " << gpuCopyMs << L" ms" << std::endl;
    std::wcout << L"  Sync Overhead: " << syncOverheadMs << L" ms" << std::endl;
    std::wcout << L"  Data Transfer (Total): " << transferMs << L" ms" << std::endl;
    std::wcout << L"  Other: " << otherMs << L" ms" << std::endl;
    std::wcout << L"  Total: " << totalMs << L" ms" << std::endl;

    return true;
}

bool Texture2D::InternalCreateFromData(const uint8_t* data, size_t dataSize, uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    // リソースを確保 (GetDefaultResourceを使用)
    m_resource = GetDefaultResource(width, height);
    if (m_resource == nullptr) {
        return false;
    }

    // データの書き込み
    try
    {
        ThrowIfFailed(m_resource->WriteToSubresource(
            0,
            nullptr,
            data,
            static_cast<UINT>(width * 4), // RowPitch: 1ラインのバイト数 (R8G8B8A8想定)
            static_cast<UINT>(dataSize)   // SlicePitch: 全体のバイト数
        ));
    }
    catch (const std::exception&)
    {
        return false;
    }

    // SRV設定の作成
    m_srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    m_srvDesc.Texture2D.MipLevels = 1;

    return true;
}

Microsoft::WRL::ComPtr<ID3D12Resource> Texture2D::GetDefaultResource(size_t width, size_t height)
{
    // R8G8B8A8_UNORM 固定でリソースを作成するヘルパー
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    auto texHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

    Microsoft::WRL::ComPtr<ID3D12Resource> buff = nullptr;

    auto result = g_Engine->Device()->CreateCommittedResource(
        &texHeapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        nullptr,
        IID_PPV_ARGS(&buff)
    );

    if (FAILED(result))
    {
        assert(SUCCEEDED(result));
        return nullptr;
    }
    return buff;
}