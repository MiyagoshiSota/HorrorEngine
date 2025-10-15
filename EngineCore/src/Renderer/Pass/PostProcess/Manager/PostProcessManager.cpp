#include "PostProcessManager.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm> // for std::min

#include "Renderer/Pass/PostProcess/Pass/VHSFilterPass.h"

using json = nlohmann::json;

// 線形補間を行うヘルパー関数
float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

PostProcessManager::PostProcessManager()
{
	m_AvailablePasses.push_back(std::make_shared<VHSFilterPass>());
}

void PostProcessManager::Init()
{
    // 初期状態として "Normal" プリセットを直接設定
    if (m_Presets.count("Normal")) {
        m_CurrentSettings = m_Presets["Normal"].settings;
    }
}

void PostProcessManager::LoadPresets(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        printf("Failed to open post-process preset file: %s\n", filePath.c_str());
        return;
    }

    json presetJson;
    file >> presetJson;

    if (!presetJson.contains("presets")) return;

    for (auto& [presetName, presetData] : presetJson["presets"].items())
    {
        PostProcessPreset preset;
        preset.name = presetName;

        for (auto& [passName, passData] : presetData.items())
        {
            PostProcessParameter params;
            for (auto& [paramName, paramValue] : passData.items())
            {
                params[paramName] = paramValue.get<float>();
            }
            preset.settings[passName] = params;
        }
        m_Presets[presetName] = preset;
    }

    printf("Post-process presets loaded successfully.\n");
}

void PostProcessManager::BlendToPreset(const std::string& presetName, float duration)
{
    if (!m_Presets.count(presetName)) {
        printf("Preset '%s' not found.\n", presetName.c_str());
        return;
    }

    // 現在のパラメータ状態をブレンド元として保存
    m_SourcePreset.settings = m_CurrentSettings;
    m_TargetPreset = m_Presets[presetName];

    m_BlendDuration = (duration > 0.0f) ? duration : 0.0f;
    m_BlendTimer = 0.0f;

    if (m_BlendDuration == 0.0f) {
        // durationが0なら即座に適用
        m_CurrentSettings = m_TargetPreset.settings;
        m_IsBlending = false;
    }
    else {
        m_IsBlending = true;
    }
}

void PostProcessManager::Update(float deltaTime)
{
    if (!m_IsBlending) return;

    m_BlendTimer += deltaTime;
    float alpha = std::min(m_BlendTimer / m_BlendDuration, 1.0f);

    // ターゲットプリセットに含まれる全てのパスとパラメータを線形補間する
    for (const auto& [passName, passParams] : m_TargetPreset.settings)
    {
        for (const auto& [paramName, targetValue] : passParams)
        {
            // ブレンド元の値を取得（存在しなければ0.0f）
            float sourceValue = 0.0f;
            if (m_SourcePreset.settings.count(passName) && m_SourcePreset.settings[passName].count(paramName)) {
                sourceValue = m_SourcePreset.settings[passName][paramName];
            }

            // 線形補間して現在の設定値を更新
            m_CurrentSettings[passName][paramName] = lerp(sourceValue, targetValue, alpha);
        }
    }

    if (alpha >= 1.0f) {
        m_IsBlending = false;
    }
}

void PostProcessManager::ExecutePasses(RenderContext& context)
{
    // 最初の入力はGeometryPassの結果である"SceneColor"
    std::shared_ptr<ITargetBase> sourceRT = context.GetRenderTarget("SceneColor");

    // 中間バッファを2つ用意
    std::shared_ptr<ITargetBase> bufferA = context.GetRenderTarget("PostProcessA");
    std::shared_ptr<ITargetBase> bufferB = context.GetRenderTarget("PostProcessB");

	// 最終出力はバックバッファ
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = g_Engine->GetCurrentRtvHandle();

	// TODO: 各パスに対してm_CurrentSettingsからパラメータを適用する処理を追加する
    for (size_t i = 0; i < m_AvailablePasses.size(); ++i)
    {
        // 最後のパスなら出力先はバックバッファ、そうでなければ中間バッファ
        if (i == m_AvailablePasses.size() - 1)
        {
			auto destRT = backBufferRTV;

            // Contextに入力元と出力先を教える
            context.SetSourceRT(sourceRT);

            // パスを実行
            m_AvailablePasses[i]->LastExecute(context,backBufferRTV);
        }else
        {
			auto destRT = (i % 2 == 0) ? bufferA : bufferB;

            // Contextに入力元と出力先を教える
            context.SetSourceRT(sourceRT);
            context.SetDestRT(destRT);

            // パスを実行
            m_AvailablePasses[i]->Execute(context);

            // 次のパスのために、今書き込んだ出力先を次の入力元にする
            sourceRT = destRT;
        }
    }
}