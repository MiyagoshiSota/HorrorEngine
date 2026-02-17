#pragma once
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Renderer/Pass/PostProcess/PostProcessPassBase.h"
#include "Renderer/Pass/PostProcess/Preset/PostProcessPreset.h"
#include "Renderer/RenderContext/RenderContext.h"

class RenderTarget;

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

    /// フレーム開始時に、アクティブなパスへ現在のプリセットパラメータを適用する
    void PreparePassesForFrame();

    /// 現在アクティブなポストプロセスパスのリストを返す
    /// （プリセットの order と AvailablePasses に基づく）
    const std::vector<std::shared_ptr<PostProcessPassBase>>& GetActivePasses() const;

public:
	std::list<std::string> GetPresetNames() const {
		std::list<std::string> names;
		for (const auto& pair : m_Presets) {
			names.push_back(pair.first);
		}
		return names;
	}

	/// 各パス出力のキャプチャを有効/無効にする
	void SetCapturePassOutputsForDebug(bool capture) { m_CapturePassOutputsForDebug = capture; }
	bool IsCapturePassOutputsForDebug() const { return m_CapturePassOutputsForDebug; }

	/// 現在のプリセットで実行されるパス名の順序
	const std::vector<std::string>& GetCurrentPresetOrder() const;

	/// 指定パス名のキャプチャ済み出力テクスチャ
	std::shared_ptr<ITargetBase> GetDebugPassOutput(const std::string& passName) const;

	/// デバッグ用パス出力のクリア
	void ClearDebugPassOutputs() { m_DebugPassOutputs.clear(); }

	/// デバッグ用にパス出力を登録する
	void SetDebugPassOutput(const std::string& passName, const std::shared_ptr<ITargetBase>& rt);

private:
    std::map<std::string, PostProcessPreset> m_Presets; // ロードした全プリセット
	std::map<std::string, std::shared_ptr<PostProcessPassBase>> m_AvailablePasses; // 利用可能な全ポストプロセスパス

    // ブレンド中の状態
    bool m_IsBlending = false;
    float m_BlendTimer = 0.0f;
    float m_BlendDuration = 1.0f;
    PostProcessPreset m_SourcePreset;
    PostProcessPreset m_TargetPreset;

    // 現在のフレームで適用すべき、最終的なパラメータ
    PostProcessPassSettings m_CurrentSettings;
	std::string m_CurrentPresetName;

	// 各パス出力のキャプチャ
	bool m_CapturePassOutputsForDebug = false;
	std::map<std::string, std::shared_ptr<RenderTarget>> m_DebugPassOutputs;

    // 現在のプリセット設定に基づくアクティブパスキャッシュ
    mutable std::vector<std::shared_ptr<PostProcessPassBase>> m_ActivePassesCache;

    // 全ポストプロセスで共有できる経過時間（秒）
    float m_Time = 0.0f;
};
