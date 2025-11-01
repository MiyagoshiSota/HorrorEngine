#include "PipelineStateManager.h"

#include <utility>

#include "Renderer/Engine.h" 
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Renderer/Graphics/RootSignatureBuilder.h"

void PipelineStateManager::create_root_signature(
    const std::string& name,
    const std::shared_ptr<RootSignatureBuilder>& builder)
{
    // 作成済みなら何もしない
    if (rootSignatureMap.count(name)) {
        return;
    }

    auto rootSignature = std::make_shared<RootSignature>();
    rootSignature->create(g_Engine->Device(), builder);

    if (!rootSignature->is_valid()) {
        printf("ルートシグネチャの生成に失敗: %s\n", name.c_str());
        return;
    }

    // マップに保存
    rootSignatureMap[name] = rootSignature;
}

void PipelineStateManager::create_pipeline_state(
    const std::string& name,
    const std::string& rootSignatureName,
    const std::wstring& vsFilePath,
    const std::wstring& psFilePath,
    bool useWireframe,
    bool useInputLayout,
    bool useDepthFormat)
{
    // 作成済みなら何もしない
    if (pipelineStateMap.count(name)) {
        return;
    }

    // 指定されたルートシグネチャを取得
    auto rootSignature = get_root_signature(rootSignatureName);
    if (rootSignature == nullptr) {
        printf("指定されたルートシグネチャが見つかりません: %s\n", rootSignatureName.c_str());
        return;
    }

    auto pipelineState = std::make_shared<PipelineState>();

    // Inputレイアウトを設定
    if (useInputLayout) {
        pipelineState->SetInputLayout(SharedStruct::Vertex::InputLayout);
    }

    pipelineState->SetWireFrame(useWireframe);
    pipelineState->SetRootSignature(rootSignature->get());
    pipelineState->SetVS(vsFilePath);
    pipelineState->SetPS(psFilePath);
    pipelineState->SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);

    // 深度ステンシルのフォーマットを設定
    if (useDepthFormat) {
        pipelineState->SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);
    }
    else {
        pipelineState->SetDepthStencilFormat(DXGI_FORMAT_UNKNOWN);
    }

    pipelineState->Create();
    if (!pipelineState->IsValid()) {
        printf("パイプラインステートの生成に失敗: %s\n", name.c_str());
        return;
    }

    // マップに保存
    pipelineStateMap[name] = pipelineState;
}

std::shared_ptr<RootSignature> PipelineStateManager::get_root_signature(const std::string& name)
{
    if (rootSignatureMap.count(name)) {
        return rootSignatureMap[name];
    }
    return nullptr;
}

std::shared_ptr<PipelineState> PipelineStateManager::get_pipeline_state(const std::string& name)
{
    if (pipelineStateMap.count(name)) {
        return pipelineStateMap[name];
    }
    return nullptr;
}