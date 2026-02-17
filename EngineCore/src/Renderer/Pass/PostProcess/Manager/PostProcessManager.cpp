#include "PostProcessManager.h"

#include <set>

#include "Renderer/Pass/PostProcess/Pass/BloomPass.h"
#include "Renderer/Target/RenderTarget.h"
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
    // ブレンド処理
    if (m_IsBlending)
    {
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

    // 経過時間の更新（全ポストプロセス共通）
    m_Time += deltaTime;

    // "g_Time" を利用するパス向けに、現在の時間をパラメータとして流し込む
    for (auto& [passName, params] : m_CurrentSettings)
    {
        params["g_Time"] = m_Time;
    }
}

void PostProcessManager::PreparePassesForFrame()
{
	const auto& order = GetCurrentPresetOrder();
	for (const auto& passName : order)
	{
		auto it = m_AvailablePasses.find(passName);
		if (it != m_AvailablePasses.end() && it->second)
		{
			PostProcessParameter params;
			if (m_CurrentSettings.count(passName))
			{
				params = m_CurrentSettings.at(passName);
			}
			it->second->SetParameters(params);
		}
	}
}

const std::vector<std::string>& PostProcessManager::GetCurrentPresetOrder() const
{
    if (m_IsBlending)
        return m_TargetPreset.order;
    if (m_Presets.count(m_CurrentPresetName))
        return m_Presets.at(m_CurrentPresetName).order;
    static const std::vector<std::string> kEmpty;
    return kEmpty;
}

std::shared_ptr<ITargetBase> PostProcessManager::GetDebugPassOutput(const std::string& passName) const
{
    auto it = m_DebugPassOutputs.find(passName);
    if (it == m_DebugPassOutputs.end()) return nullptr;
    return it->second;
}

void PostProcessManager::SetDebugPassOutput(const std::string& passName, const std::shared_ptr<ITargetBase>& rt)
{
    if (!m_CapturePassOutputsForDebug) return;
    if (!rt) return;

    // RenderTarget にキャストできるものだけを記録する
    auto renderTarget = std::dynamic_pointer_cast<RenderTarget>(rt);
    if (!renderTarget) return;

    m_DebugPassOutputs[passName] = renderTarget;
}

const std::vector<std::shared_ptr<PostProcessPassBase>>& PostProcessManager::GetActivePasses() const
{
    m_ActivePassesCache.clear();

    // 現在有効なプリセットのパス順序を取得
    const auto& order = m_IsBlending
        ? m_TargetPreset.order
        : (m_Presets.count(m_CurrentPresetName) ? m_Presets.at(m_CurrentPresetName).order : std::vector<std::string>{});

    for (const auto& passName : order)
    {
        auto it = m_AvailablePasses.find(passName);
        if (it != m_AvailablePasses.end() && it->second)
        {
            m_ActivePassesCache.push_back(it->second);
        }
    }

    return m_ActivePassesCache;
}