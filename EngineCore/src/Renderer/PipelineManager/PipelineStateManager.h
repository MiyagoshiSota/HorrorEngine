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
    void create_root_signature(
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
    void create_pipeline_state(
        const std::string& name,                  // 作成するPSOの名前
        const std::string& rootSignatureName,     // 使用するルートシグネチャの名前
        const std::wstring& vsFilePath,           // 頂点シェーダーのパス
        const std::wstring& psFilePath,           // ピクセルシェーダーのパス
        bool useWireframe,                        // ワイヤーフレームモードを使用するか
        bool useInputLayout,                      // InputLayoutを使用するか
        bool useDepthFormat,                      // 深度フォーマットを使用するか
        bool blendEnable                          // ブレンドを使用するか
    );
    
    void create_pipeline_state(
        const std::string& name,                  // 作成するPSOの名前
        const std::string& rootSignatureName,     // 使用するルートシグネチャの名前
        const std::wstring& vsFilePath,           // 頂点シェーダーのパス
        const std::wstring& psFilePath,           // ピクセルシェーダーのパス
        const std::wstring& gsFilePath,           // ジオメトリシェーダーのパス
        bool useWireframe,                        // ワイヤーフレームモードを使用するか
        bool useInputLayout,                      // InputLayoutを使用するか
        bool useDepthFormat,                       // 深度フォーマットを使用するか
        bool blendEnable                          // ブレンドを使用するか
    );
    
    // Computeシェーダー用のパイプラインステートを作成して登録する
    void create_pipeline_state(
        const std::string& name,                  // 作成するPSOの名前
        const std::string& rootSignatureName,     // 使用するルートシグネチャの名前
        const std::wstring& csFilePath            // コンピュートシェーダーのパス
    );

    /// <summary>
    /// 登録済みのルートシグネチャを取得する
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    std::shared_ptr<RootSignature> get_root_signature(const std::string& name);

    /// <summary>
    /// 登録済みのパイプラインステートを取得する
    /// </summary>
    /// <param name="name"></param>
    /// <returns></returns>
    std::shared_ptr<PipelineState> get_pipeline_state(const std::string& name);

private:
    std::unordered_map<std::string, std::shared_ptr<RootSignature>> rootSignatureMap;
    std::unordered_map<std::string, std::shared_ptr<PipelineState>> pipelineStateMap;
};