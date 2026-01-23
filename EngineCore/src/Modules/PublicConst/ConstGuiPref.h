#pragma once

class ConstGuiPref
{
public:
    // レイアウトプリセット関連のディレクトリ
    static inline const char* kDefaultPresetDirectory = "Config/GUI/";
    
    // デフォルトプリセットファイル名（テンプレート）
    static inline const char* kLayoutMakeModeDefaultFile = "layout_makemode.ini";
    static inline const char* kLayoutDebugModeDefaultFile = "layout_debugmode.ini";
    
    // 設定ファイル名
    static inline const char* kLayoutConfigFile = "layout_config.json";
    
    // プリセット名（JSON保存用）
    static inline const char* kPresetNameMakeMode = "makemode";
    static inline const char* kPresetNameDebugMode = "debugmode";
};
