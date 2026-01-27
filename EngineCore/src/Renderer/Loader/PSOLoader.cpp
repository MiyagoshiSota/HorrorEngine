#include "PSOLoader.h"
#include "Renderer/PipelineManager/PipelineStateManager.h"
#include "Renderer/Graphics/RootSignatureBuilder.h"
#include "Renderer/Engine.h" // g_Engine を使うため
#include <nlohmann/json.hpp>
#include <fstream>
#include <d3dcompiler.h> // シェーダーをファイルから読み込むため
#include <filesystem>
#include <Windows.h>

#include "Modules/Other/EngineString.h"
#pragma comment(lib, "d3dcompiler.lib")

using json = nlohmann::json;
namespace fs = std::filesystem;

// 実行ファイルのディレクトリを取得
static std::filesystem::path GetExecutableDirectory()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return std::filesystem::path(exePath).parent_path();
}

// シェーダーパスを解決（相対パスを実行ファイルからのパスに変換）
static std::wstring ResolveShaderPath(const std::wstring& shaderPath)
{
    // 既に絶対パスの場合はそのまま返す
    if (std::filesystem::path(shaderPath).is_absolute())
    {
        if (std::filesystem::exists(shaderPath))
        {
            return shaderPath;
        }
    }
    
    std::filesystem::path exeDir = GetExecutableDirectory();
    
    // ファイル名を抽出（パスから最後のファイル名部分を取得）
    std::wstring filename;
    size_t filenameStart = shaderPath.find_last_of(L"/\\");
    if (filenameStart != std::wstring::npos)
    {
        filename = shaderPath.substr(filenameStart + 1);
    }
    else
    {
        filename = shaderPath;
    }
    
    // 試すパスのリスト（優先順位順）
    std::vector<std::filesystem::path> candidatePaths;
    
    // 1. Assets/Shaders/ (ビルド済みexe用) - 最優先
    candidatePaths.push_back(exeDir / L"Assets" / L"Shaders" / filename);
    
    // 2. 実行ファイルのディレクトリ直下 (開発環境の場合、シェーダーが同じディレクトリにある)
    candidatePaths.push_back(exeDir / filename);
    
    // 3. 実行ファイルのディレクトリの親/Assets/Shaders/ (開発環境の場合)
    if (exeDir.has_parent_path())
    {
        candidatePaths.push_back(exeDir.parent_path() / L"Assets" / L"Shaders" / filename);
    }
    
    // 4. 作業ディレクトリ/Assets/Shaders/ (開発環境の場合)
    // Visual Studioの作業ディレクトリがGameに設定されている場合
    wchar_t currentDir[MAX_PATH];
    if (GetCurrentDirectoryW(MAX_PATH, currentDir) > 0)
    {
        std::filesystem::path workDir = currentDir;
        candidatePaths.push_back(workDir / L"Assets" / L"Shaders" / filename);
    }
    
    // 5. 元のパスをそのまま試す（../x64/Debug/など）- 開発環境用のフォールバック
    // 実行ファイルのディレクトリからの相対パスとして解決
    std::filesystem::path originalPath = exeDir / shaderPath;
    candidatePaths.push_back(originalPath);
    
    // 6. 実行ファイルのディレクトリの親からの元のパス（開発環境の場合）
    // 例: Game/x64/Debug/Game.exe の場合、Game/../x64/Debug/SimpleVS.cso = x64/Debug/SimpleVS.cso
    if (exeDir.has_parent_path())
    {
        std::filesystem::path parentPath = exeDir.parent_path() / shaderPath;
        candidatePaths.push_back(parentPath);
        
        // 正規化して試す
        try
        {
            std::filesystem::path normalizedPath = std::filesystem::canonical(parentPath);
            candidatePaths.push_back(normalizedPath);
        }
        catch (...)
        {
            // 正規化に失敗した場合は無視
        }
    }
    
    // 7. 作業ディレクトリからの相対パス（開発環境の場合）
    if (GetCurrentDirectoryW(MAX_PATH, currentDir) > 0)
    {
        std::filesystem::path workDir = currentDir;
        candidatePaths.push_back(workDir / shaderPath);
    }
    
    // 各候補パスを試す
    for (const auto& candidate : candidatePaths)
    {
        if (std::filesystem::exists(candidate))
        {
            return candidate.wstring();
        }
    }
    
    // デバッグ用: 試したパスを出力
    printf("Shader path resolution failed for: %ls\n", shaderPath.c_str());
    printf("Tried the following paths:\n");
    for (size_t i = 0; i < candidatePaths.size(); ++i)
    {
        char pathBuffer[512];
        WideCharToMultiByte(CP_UTF8, 0, candidatePaths[i].wstring().c_str(), -1, pathBuffer, sizeof(pathBuffer), nullptr, nullptr);
        printf("  [%zu] %s\n", i + 1, pathBuffer);
    }
    
    // 見つからない場合は最初の候補を返す（エラーはPipelineStateで処理）
    return candidatePaths[0].wstring();
}

// --- JSONの文字列をDirectXのenumに変換するヘルパー関数群 ---
// (別のユーティリティファイルにまとめても良い)
namespace JsonParserHelpers
{
    D3D12_FILTER ParseFilter(const std::string& str) {
        if (str == "MIN_MAG_MIP_POINT") return D3D12_FILTER_MIN_MAG_MIP_POINT;
        else if (str == "MIN_MAG_POINT_MIP_LINEAR") return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        else if (str == "MIN_POINT_MAG_LINEAR_MIP_POINT") return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        else if (str == "MIN_POINT_MAG_MIP_LINEAR") return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        else if (str == "MIN_LINEAR_MAG_MIP_POINT") return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else if (str == "MIN_LINEAR_MAG_POINT_MIP_LINEAR") return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        else if (str == "MIN_MAG_LINEAR_MIP_POINT") return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        else if (str == "MIN_MAG_MIP_LINEAR") return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        else if (str == "ANISOTROPIC") return D3D12_FILTER_ANISOTROPIC;
        return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	}

    D3D12_TEXTURE_ADDRESS_MODE ParseAddressMode(const std::string& str) {
        if (str == "WRAP") return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        else if (str == "MIRROR") return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        else if (str == "CLAMP") return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        else if (str == "BORDER") return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        else if (str == "MIRROR_ONCE") return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }

    D3D12_ROOT_SIGNATURE_FLAGS ParseRootSignatureFlags(const json& flagsArray) {
        D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        for (const auto& flagStr : flagsArray) {
            if (flagStr == "ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT") flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            else if (flagStr == "DENY_HULL_SHADER_ROOT_ACCESS") flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
            else if (flagStr == "DENY_DOMAIN_SHADER_ROOT_ACCESS") flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
            else if (flagStr == "DENY_GEOMETRY_SHADER_ROOT_ACCESS") flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
        }
        return flags;
    }

    D3D12_COMPARISON_FUNC ParseComparisonFunc(const std::string& str) {
        if (str == "NEVER") return D3D12_COMPARISON_FUNC_NEVER;
        else if (str == "LESS") return D3D12_COMPARISON_FUNC_LESS;
        else if (str == "EQUAL") return D3D12_COMPARISON_FUNC_EQUAL;
        else if (str == "LESS_EQUAL") return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        else if (str == "GREATER") return D3D12_COMPARISON_FUNC_GREATER;
        else if (str == "NOT_EQUAL") return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        else if (str == "GREATER_EQUAL") return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        else if (str == "ALWAYS") return D3D12_COMPARISON_FUNC_ALWAYS;
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

    D3D12_STATIC_BORDER_COLOR ParseBorderColor(const std::string& str) {
        if (str == "TRANSPARENT_BLACK") return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        else if (str == "OPAQUE_BLACK") return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        else if (str == "OPAQUE_WHITE") return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	}
}


std::shared_ptr<PipelineStateManager> PSOLoader::LoadFromFile(const std::string& filePath, std::shared_ptr<PipelineStateManager> manager)
{
    // JSONファイルを読み込んでパース
    std::ifstream file(filePath);
    if (!file.is_open()) {
        printf("Failed to open PSO definition file: %s\n", filePath.c_str());
        return nullptr;
    }
    json psoJson;
    file >> psoJson;

    auto device = g_Engine->Device();

    // RootSignaturesセクションを解析して、先に全て生成する
    if (psoJson.contains("RootSignatures")) {
        for (auto& [name, rsJson] : psoJson["RootSignatures"].items()) {
            auto builder = std::make_shared<RootSignatureBuilder>();
            // CBV
            if (rsJson.contains("constantBufferViews")) {
                for (const auto& cbv : rsJson["constantBufferViews"]) {
                    builder->AddConstantBufferView(cbv.value("shaderRegister", 0));
                }
            }
            // UAV
            if (rsJson.contains("unorderedAccessViews")) {
                for (const auto& uav : rsJson["unorderedAccessViews"]) {
                    builder->AddUnorderedAccessView(uav.value("shaderRegister", 0));
                }
            }
            // SRV
            if (rsJson.contains("shaderResourceViews")) {
                for (const auto& srv : rsJson["shaderResourceViews"]) {
                    builder->AddShaderResourceView(srv.value("shaderRegister", 0));
                }
            }
            // Descriptor Tables
            if (rsJson.contains("descriptorTables")) {
                for (const auto& table : rsJson["descriptorTables"]) {
                    CD3DX12_DESCRIPTOR_RANGE range(
                        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                        table.value("num", 1),
                        table.value("shaderRegister", 0)
                    );
                    builder->AddDescriptorTable(1, &range);
                }
            }
            // Static Samplers
            if (rsJson.contains("staticSamplers")) {
                for (const auto& sampler : rsJson["staticSamplers"]) {

                    // ヘルパー関数等で文字列からenumへ変換すると仮定
                    auto filter = JsonParserHelpers::ParseFilter(sampler.value("filter", "LINEAR"));
                    auto addressMode = JsonParserHelpers::ParseAddressMode(sampler.value("addressU", "WRAP")); // U,V,W共通で簡略化
                    auto comparisonFunc = JsonParserHelpers::ParseComparisonFunc(sampler.value("comparisonFunc", "NEVER"));
                    auto borderColor = JsonParserHelpers::ParseBorderColor(sampler.value("borderColor", "TRANSPARENT_BLACK"));

                    CD3DX12_STATIC_SAMPLER_DESC samplerDesc(
                        sampler.value("shaderRegister", 0),
                        filter,
                        addressMode, // AddressU
                        addressMode, // AddressV
                        addressMode, // AddressW
                        0.0f,        // MipLODBias
                        16,          // MaxAnisotropy
                        comparisonFunc, // LESS_EQUALなどを渡す
                        borderColor     // OPAQUE_WHITEなどを渡す
                    );

                    builder->AddStaticSampler(samplerDesc);
                }
            }
            // Flags
            if (rsJson.contains("flags")) {
                builder->SetFlags(JsonParserHelpers::ParseRootSignatureFlags(rsJson["flags"]));
            }

            // マネージャーにルートシグネチャを生成・登録させる
            manager->CreateRootSignature(name, builder);
        }
    }

    // 3. PipelineStatesセクションを解析してPSOを生成する
    if (psoJson.contains("PipelineStates")) {
        for (auto& [name, psoJson] : psoJson["PipelineStates"].items()) {
            std::string rootSignatureName = psoJson["rootSignature"];
            // グラフィックスシェーダー用PSO
            if (psoJson.contains("vertexShader") || psoJson.contains("pixelShader"))
            {
                std::wstring vsPath = L"";
                std::wstring psPath = L"";
                std::wstring gsPath = L"";
	            
            	if (psoJson.contains("vertexShader"))
	            {
                    vsPath = ResolveShaderPath(EngineString::to_wstring(psoJson["vertexShader"]));
	            }

                if (psoJson.contains("pixelShader"))
                {
                	psPath = ResolveShaderPath(EngineString::to_wstring(psoJson["pixelShader"]));
                }

                if (psoJson.contains("geometryShader"))
                {
                    gsPath = ResolveShaderPath(EngineString::to_wstring(psoJson["geometryShader"]));
                }

				UINT SampleCount = psoJson.value("sampleCount", 1);

				// format指定がある場合の処理
                DXGI_FORMAT renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                if (psoJson.contains("format"))
                {
                    std::string formatStr = psoJson["format"];
                    if (formatStr == "DXGI_FORMAT_R8G8B8A8_UNORM") {
                        renderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                    }
                    else if (formatStr == "DXGI_FORMAT_B8G8R8A8_UNORM") {
                        renderTargetFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
                    }
                    else if (formatStr == "DXGI_FORMAT_R16G16B16A16_FLOAT"){
                        renderTargetFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
					}
                }

                bool depthEnable = psoJson.value("depthEnable", true);
                bool inputLayout = psoJson.value("inputLayoutEnable", false);
                bool useWireframe = psoJson.value("wireframeEnable", false);
                bool blendEnable = psoJson.value("blendEnable", false);

                // 深度比較関数のパース
                D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;
                if (psoJson.contains("depthFunc"))
                {
                    depthFunc = JsonParserHelpers::ParseComparisonFunc(psoJson["depthFunc"]);
                }

                // マネージャーにPSOを生成・登録させる

                // ジオメトリシェーダー有りの場合
                if(!gsPath.empty())
                {
                    manager->CreatePipelineState(name, rootSignatureName,vsPath, psPath, gsPath, useWireframe,inputLayout, depthEnable, blendEnable, depthFunc);
                }
                // ジオメトリシェーダー無しの場合
                manager->CreatePipelineState(name, rootSignatureName,vsPath, psPath, SampleCount, renderTargetFormat, useWireframe,inputLayout, depthEnable, blendEnable, depthFunc);
            }
            // Computeシェーダー用PSO
            else if (psoJson.contains("computeShader"))
            {
                std::wstring csPath = ResolveShaderPath(EngineString::to_wstring(psoJson["computeShader"]));

                manager->CreatePipelineState(name, rootSignatureName, csPath);
            }
        }
    }

    return manager;
}