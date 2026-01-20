#pragma once

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "Modules/PublicConst/const_gui_pref.h"
#include "LayoutPresetType.h"

// 前方宣言
class Engine;
class IDrawWindow;

using json = nlohmann::json;

class LayoutPresetManager
{
public:
    static LayoutPresetManager& GetInstance()
    {
        static LayoutPresetManager instance;
        return instance;
    }

    // プリセットを適用（ウィンドウの表示/非表示のみ設定）
    void ApplyPreset(LayoutPresetType presetType);

    // プリセットのレイアウトを適用（プリセットファイルをGame/imgui.iniにコピー）
    // NewFrame()の前に呼ぶ必要がある
    void LoadPresetLayout(LayoutPresetType presetType);

    // 現在のプリセットを取得
    LayoutPresetType GetCurrentPreset() const { return m_currentPreset; }

    // 前回選択したプリセットを読み込み
    LayoutPresetType LoadLastPreset();

    // 現在のプリセットを保存
    void SaveCurrentPreset(LayoutPresetType presetType);
    
    // デフォルトプリセットファイルのパスを取得（assetsから読み込む）
    std::string GetDefaultPresetFilePath(LayoutPresetType presetType) const;
    
    // 設定ファイルのパスを取得
    std::string GetConfigFilePath() const;

private:
    LayoutPresetManager() = default;
    ~LayoutPresetManager() = default;
    LayoutPresetManager(const LayoutPresetManager&) = delete;
    LayoutPresetManager& operator=(const LayoutPresetManager&) = delete;

    // ウィンドウの表示/非表示を設定
    void SetWindowVisibility(LayoutPresetType presetType);

    LayoutPresetType m_currentPreset = LayoutPresetType::MakeMode;
    std::string m_configDir = "config/gui/";
    std::string m_imguiIniPath = "imgui.ini";  // Game/imgui.ini
};
