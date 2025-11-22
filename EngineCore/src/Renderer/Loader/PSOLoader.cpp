#include "PSOLoader.h"
#include "Renderer/PipelineManager/PipelineStateManager.h"
#include "Renderer/Graphics/RootSignatureBuilder.h"
#include "Renderer/Engine.h" // g_Engine を使うため
#include <nlohmann/json.hpp>
#include <fstream>
#include <d3dcompiler.h> // シェーダーをファイルから読み込むため

#include "Modules/Other/engineString.h"
#pragma comment(lib, "d3dcompiler.lib")

using json = nlohmann::json;

// --- JSONの文字列をDirectXのenumに変換するヘルパー関数群 ---
// (別のユーティリティファイルにまとめても良い)
namespace JsonParserHelpers
{
    D3D12_FILTER ParseFilter(const std::string& str) {
        if (str == "LINEAR") return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        // ... 他のフィルタータイプも同様に追加 ...
        return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    }

    D3D12_TEXTURE_ADDRESS_MODE ParseAddressMode(const std::string& str) {
        if (str == "WRAP") return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        // ... 他のアドレスモードも同様に追加 ...
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
}


std::shared_ptr<PipelineStateManager> PSOLoader::load_from_file(const std::string& filePath, std::shared_ptr<PipelineStateManager> manager)
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
                    builder->add_constant_buffer_view(cbv.value("shaderRegister", 0));
                }
            }
            // UAV
            if (rsJson.contains("unorderedAccessViews")) {
                for (const auto& uav : rsJson["unorderedAccessViews"]) {
                    builder->add_unordered_access_view(uav.value("shaderRegister", 0));
                }
            }
            // SRV
            if (rsJson.contains("shaderResourceViews")) {
                for (const auto& srv : rsJson["shaderResourceViews"]) {
                    builder->add_shader_resource_view(srv.value("shaderRegister", 0));
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
                    builder->add_descriptor_table(1, &range);
                }
            }
            // Static Samplers
            if (rsJson.contains("staticSamplers")) {
                for (const auto& sampler : rsJson["staticSamplers"]) {
                    CD3DX12_STATIC_SAMPLER_DESC samplerDesc(
                        sampler.value("shaderRegister", 0),
                        JsonParserHelpers::ParseFilter(sampler.value("filter", "LINEAR"))
                    );
                    builder->add_static_sampler(samplerDesc);
                }
            }
            // Flags
            if (rsJson.contains("flags")) {
                builder->set_flags(JsonParserHelpers::ParseRootSignatureFlags(rsJson["flags"]));
            }

            // マネージャーにルートシグネチャを生成・登録させる
            manager->create_root_signature(name, builder);
        }
    }

    // 3. PipelineStatesセクションを解析してPSOを生成する
    if (psoJson.contains("PipelineStates")) {
        for (auto& [name, psoJson] : psoJson["PipelineStates"].items()) {
            std::string rootSignatureName = psoJson["rootSignature"];
            // グラフィックスシェーダー用PSO
            if (psoJson.contains("vertexShader") && psoJson.contains("pixelShader"))
            {
                std::wstring vsPath = engine_string::to_wstring(psoJson["vertexShader"]);
                std::wstring psPath = engine_string::to_wstring(psoJson["pixelShader"]);
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

				std::wstring gsPath = L"";

                if (psoJson.contains("geometryShader"))
                {
                    gsPath = engine_string::to_wstring(psoJson["geometryShader"]);
                }

                bool depthEnable = psoJson.value("depthEnable", true);
                bool inputLayout = psoJson.value("inputLayoutEnable", false);
                bool useWireframe = psoJson.value("wireframeEnable", false);
                bool blendEnable = psoJson.value("blendEnable", false);

                // マネージャーにPSOを生成・登録させる

                // ジオメトリシェーダー有りの場合
                if(!gsPath.empty())
                {
                    manager->create_pipeline_state(name, rootSignatureName,vsPath, psPath, gsPath, useWireframe,inputLayout, depthEnable, blendEnable);
                }

                // ジオメトリシェーダー無しの場合
                manager->create_pipeline_state(name, rootSignatureName,vsPath, psPath, SampleCount, renderTargetFormat, useWireframe,inputLayout, depthEnable, blendEnable);
            }
            // Computeシェーダー用PSO
            else if (psoJson.contains("computeShader"))
            {
                std::wstring csPath = engine_string::to_wstring(psoJson["computeShader"]);

                manager->create_pipeline_state(name, rootSignatureName, csPath);
            }
        }
    }

    return manager;
}