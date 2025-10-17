#include "PostProcessManager.h"

#include "Modules/PublicConst/const_render_pref.h"
#include "Renderer/Pass/PostProcess/Pass/MonochromePass.h"
#include "Renderer/Pass/PostProcess/Pass/VHSPass.h"

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
}

void PostProcessManager::Init()
{
	// TEST:デフォルトプリセットを"Flashback"に設定
    if (m_Presets.count("Flashback")) {
        m_CurrentSettings = m_Presets["Flashback"].settings;
        m_CurrentPresetName = "Flashback";
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
        preset.name = presetName;

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
	// 現在のプリセット名を更新
    m_SourcePreset.settings = m_CurrentSettings;
    m_TargetPreset = m_Presets[presetName];

	// ブレンドの初期化
    m_BlendDuration = (duration > 0.0f) ? duration : 0.0f;
    m_BlendTimer = 0.0f;

	// BlendDurationが0なら即座に切り替え
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

	// 各パスの各パラメータを線形補間
    for (const auto& [passName, passParams] : m_TargetPreset.settings)
    {
        for (const auto& [paramName, targetValue] : passParams)
        {
            float sourceValue = 0.0f;

			// ソースプリセットに値があればそれを使う
            if (m_SourcePreset.settings.count(passName) && m_SourcePreset.settings[passName].count(paramName)) {
                sourceValue = m_SourcePreset.settings[passName][paramName];
            }

			// 現在の設定を更新
            m_CurrentSettings[passName][paramName] = lerp(sourceValue, targetValue, alpha);
        }
    }

	// ブレンドが完了したらフラグを下ろす
    if (alpha >= 1.0f) {
        m_IsBlending = false;
    }
}

void PostProcessManager::ExecutePasses(RenderContext& context)
{
    // 現在のプリセット（ブレンド中ならターゲットプリセット）で有効なパスのリストを取得
    const auto& activePassesOrder = m_IsBlending ? m_TargetPreset.order : m_Presets[m_CurrentPresetName].order;
    if (activePassesOrder.empty()) return;

    // 最初の入力はGeometryPassの結果である"SceneColor"
    std::shared_ptr<ITargetBase> sourceRT = context.GetRenderTarget(const_render_pref::SceneColor);

    // 中間バッファを2つ用意
    std::shared_ptr<ITargetBase> bufferA = context.GetRenderTarget(const_render_pref::TmpColorA);
    std::shared_ptr<ITargetBase> bufferB = context.GetRenderTarget(const_render_pref::TmpColorB);

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