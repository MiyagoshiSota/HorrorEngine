#pragma once
#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"
#include "Renderer/Pass/PostProcess/Preset/PostProcessPreset.h"
#include "Renderer/RenderContext/RenderContext.h"

class PostProcessManager
{
public:
	PostProcessManager();

    void Init();
    void LoadPresets(const std::string& filePath);

    // ブレンドを開始する
    void BlendToPreset(const std::string& presetName, float duration);

    // 毎フレーム呼び出す
    void Update(float deltaTime);

    // 描画時に呼び出す
    void ExecutePasses(RenderContext& context);

private:
    std::map<std::string, PostProcessPreset> m_Presets; // ロードした全プリセット
	std::vector<std::shared_ptr<PostProcessPassBase>> m_AvailablePasses; // 利用可能な全ポストプロセスパス

    // ブレンド中の状態
    bool m_IsBlending = false;
    float m_BlendTimer = 0.0f;
    float m_BlendDuration = 1.0f;
    PostProcessPreset m_SourcePreset;
    PostProcessPreset m_TargetPreset;

    // 現在のフレームで適用すべき、最終的なパラメータ
    PostProcessPassSettings m_CurrentSettings;
};
