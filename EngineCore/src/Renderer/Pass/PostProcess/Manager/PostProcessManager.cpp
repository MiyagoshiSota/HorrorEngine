#include "PostProcessManager.h"

#include <set>

#include "Modules/PublicConst/ConstRenderPref.h"
#include "Renderer/Pass/PostProcess/Pass/BloomPass.h"
#include "Renderer/Pass/PostProcess/Pass/ChromaticAberration.h"
#include "Renderer/Pass/PostProcess/Pass/FilmGrainPass.h"
#include "Renderer/Pass/PostProcess/Pass/MonochromePass.h"
#include "Renderer/Pass/PostProcess/Pass/VHSPass.h"
#include "Renderer/Pass/PostProcess/Pass/VignettePass.h"

using json = nlohmann::json;

// 線形補間を行うヘルパー関数
float lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

PostProcessManager::PostProcessManager()
{
    m_AvailablePasses["VHS"] = std::make_shared<VHSPass>();
    m_AvailablePasses["Monochrome"] = std::make_shared<MonochromePass>();
	m_AvailablePasses["Vignette"] = std::make_shared<VignettePass>();
	m_AvailablePasses["FilmGrain"] = std::make_shared<FilmGrainPass>();
	m_AvailablePasses["ChromaticAberration"] = std::make_shared<ChromaticAberration>();
	m_AvailablePasses["Bloom"] = std::make_shared<Bloom>();
}

void PostProcessManager::Init()
{
	// TEST:デフォルトプリセットを"Flashback"に設定
    if (m_Presets.count("Normal")) {
        m_CurrentSettings = m_Presets["Normal"].settings;
        m_CurrentPresetName = "Normal";
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

    // JSONからプリセットを読み込む
    for (auto& [presetName, presetData] : presetJson["presets"].items())
    {
        PostProcessPreset preset;
        preset.m_name = presetName;

		// パスの順序を読み込む
        if (presetData.contains("order")) {
            for (const auto& passName : presetData["order"]) {
                preset.order.push_back(passName.get<std::string>());
            }
        }

		// 各パスのパラメータを読み込む
        for (auto& [passName, passData] : presetData["settings"].items())
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

    // 常に現在のプリセット名も更新する
    m_CurrentPresetName = presetName;

    // 現在のパラメータ設定をソースとしてディープコピー
    m_SourcePreset.settings = m_CurrentSettings;
    m_TargetPreset = m_Presets[presetName];

    m_BlendDuration = (duration > 0.0f) ? duration : 0.0f;
    m_BlendTimer = 0.0f;

    if (m_BlendDuration == 0.0f) {
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
    
    // ソースとターゲットに存在する全てのユニークなパス名を収集
    std::set<std::string> allPassNames;
    for (const auto& [passName, _] : m_SourcePreset.settings) {
        allPassNames.insert(passName);
    }
    for (const auto& [passName, _] : m_TargetPreset.settings) {
        allPassNames.insert(passName);
    }

    // 全てのパスに対してブレンド処理を行う
    for (const auto& passName : allPassNames)
    {
        // ターゲットに存在する全パラメータをループ
        const auto& targetParams = m_TargetPreset.settings[passName];
        for (const auto& [paramName, targetValue] : targetParams)
        {
            float sourceValue = 0.0f;
            if (m_SourcePreset.settings.count(passName) && m_SourcePreset.settings.at(passName).count(paramName)) {
                sourceValue = m_SourcePreset.settings.at(passName).at(paramName);
            }
            m_CurrentSettings[passName][paramName] = lerp(sourceValue, targetValue, alpha);
        }

        // ソースにしか存在しないパラメータをフェードアウトさせる
        const auto& sourceParams = m_SourcePreset.settings[passName];
        for (const auto& [paramName, sourceValue] : sourceParams)
        {
            if (!m_TargetPreset.settings.count(passName) || !m_TargetPreset.settings.at(passName).count(paramName))
            {
                // ターゲットに存在しないパラメータは0に向かって補間
                m_CurrentSettings[passName][paramName] = lerp(sourceValue, 0.0f, alpha);
            }
        }
    }

    if (alpha >= 1.0f) {
        m_IsBlending = false;
        // ブレンド完了時は、ターゲットの状態を正確にコピー
        m_CurrentSettings = m_TargetPreset.settings;
    }
}

void PostProcessManager::ExecutePasses(RenderContext& context)
{
    // 現在のプリセット（ブレンド中ならターゲットプリセット）で有効なパスのリストを取得
    const auto& activePassesOrder = m_IsBlending ? m_TargetPreset.order : m_Presets[m_CurrentPresetName].order;
    if (activePassesOrder.empty()) return;

    // 最初の入力はGeometryPassの結果である"SceneColor"
    std::shared_ptr<ITargetBase> sourceRT = context.GetRenderTarget(ConstRenderPref::SceneColor);

    // 中間バッファを2つ用意
    std::shared_ptr<ITargetBase> bufferA = context.GetRenderTarget(ConstRenderPref::TmpColorA);
    std::shared_ptr<ITargetBase> bufferB = context.GetRenderTarget(ConstRenderPref::TmpColorB);

    for (size_t i = 0; i < activePassesOrder.size(); ++i)
    {
        const std::string& passName = activePassesOrder[i];

        // 実行すべきパスオブジェクトを取得
        if (!m_AvailablePasses.count(passName)) continue;
        auto& pass = m_AvailablePasses[passName];

        // 現在のブレンド状態から、このパス用のパラメータを取得
        const auto& params = m_CurrentSettings[passName];
        pass->SetParameters(params);

        // 最後のパスなら出力先はバックバッファ、そうでなければ中間バッファ
        bool isLastPass = (i == activePassesOrder.size() - 1);
        if (isLastPass)
        {
			// バックバッファのRTVハンドルを取得
            D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = g_Engine->GetCurrentRtvHandle();

            // Contextに入力元と出力先を教える
            context.SetSourceRT(sourceRT);

            // パスを実行
            pass->LastExecute(context, backBufferRTV);
        }
        else
        {
            // 出力先のtmpを決定
            std::shared_ptr<ITargetBase> destRT = (i % 2 == 0) ? bufferA : bufferB;
            context.SetSourceRT(sourceRT);
            context.SetDestRT(destRT);

			// パスを実行
			pass->Execute(context);
        }

        // 次のパスのために、今書き込んだ出力先を次の入力元にする
        if (!isLastPass) {
            sourceRT = context.GetDestRT();
        }
    }
}