#pragma once
#include <d3d12.h>
#include <map>
#include <unordered_map>

#include "Renderer/Graphics/PipelineState.h"
#include "Renderer/Graphics/RootSignature.h"

class RootSignatureBuilder;

class PipelineStateManager
{
public:
    /// <summary>
    /// 新しいルートシグネチャを作成して登録する
    /// </summary>
    /// <param name="name"></param>
    /// <param name="builder"></param>
    void CreateRootSignature(
        const std::string& name,
        const std::shared_ptr<RootSignatureBuilder>& builder);

    /// <summary>
    /// 新しいパイプラインステートを作成して登録する
    /// </summary>
    /// <param name="name"></param>
    /// <param name="rootSignatureName"></param>
    /// <param name="vsFilePath"></param>
    /// <param name="psFilePath"></param>
    /// <param name="useInputLayout"></param>
    /// <param name="useDepthFormat"></param>
    void CreatePipelineState(
        const std::string& name,                  // 作成するPSOの名前
        const std::string& rootSignatureName,     // 使用するルートシグネチャの名前
        const std::wstring& vsFilePath,           // 頂点シェーダーのパス
        const std::wstring& psFilePath,           // ピクセルシェーダーのパス
		UINT SampleCount,                         // サンプル数
		DXGI_FORMAT renderTargetFormat,           // レンダーターゲットのフォーマット
        bool useWireframe,                        // ワイヤーフレームモードを使用するか
        bool useInputLayout,                      // InputLayoutを使用するか
        bool useDepthFormat,                      // 深度フォーマットを使用するか
        bool blendEnable,                         // ブレンドを使用するか
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS // 深度比較関数
    );
    
    // MRT対応版
    void CreatePipelineState(
        const std::string& name,                  // 作成するPSOの名前
        const std::string& rootSignatureName,     // 使用するルートシグネチャの名前
        const std::wstring& vsFilePath,           // 頂点シェーダーのパス
        const std::wstring& psFilePath,           // ピクセルシェーダーのパス
		UINT SampleCount,                         // サンプル数
		const std::vector<DXGI_FORMAT>& renderTargetFormats, // レンダーターゲットのフォーマット（MRT対応）
        bool useWireframe,                        // ワイヤーフレームモードを使用するか
        bool useInputLayout,                      // InputLayoutを使用するか
        bool useDepthFormat,                      // 深度フォーマットを使用するか
        bool blendEnable,                         // ブレンドを使用するか
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS // 深度比較関数
    );
    
    void CreatePipelineState(
        const std::string& name,                  // 作成するPSOの名前
        const std::string& rootSignatureName,     // 使用するルートシグネチャの名前
        const std::wstring& vsFilePath,           // 頂点シェーダーのパス
        const std::wstring& psFilePath,           // ピクセルシェーダーのパス
        const std::wstring& gsFilePath,           // ジオメトリシェーダーのパス
        bool useWireframe,                        // ワイヤーフレームモードを使用するか
        bool useInputLayout,                      // InputLayoutを使用するか
        bool useDepthFormat,                      // 深度フォーマットを使用するか
        bool blendEnable,                         // ブレンドを使用するか
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS // 深度比較関数
    );
    
    // Computeシェーダー用のパイプラインステートを作成して登録する
    void CreatePipelineState(
        const std::string& name,                  // 作成するPSOの名前
        const std::string& rootSignatureName,     // 使用するルートシグネチャの名前
        const std::wstring& csFilePath            // コンピュートシェーダーのパス
    );

    /// <summary>
    /// 登録済みのルートシグネチャを取得する
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    std::shared_ptr<RootSignature> GetRootSignature(const std::string& name);

    /// <summary>
    /// 登録済みのパイプラインステートを取得する
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    std::shared_ptr<PipelineState> GetPipelineState(const std::string& name);

private:
    std::unordered_map<std::string, std::shared_ptr<RootSignature>> rootSignatureMap;
    std::unordered_map<std::string, std::shared_ptr<PipelineState>> pipelineStateMap;
};