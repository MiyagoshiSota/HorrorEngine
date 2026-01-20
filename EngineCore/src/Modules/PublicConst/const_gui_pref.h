#pragma once

class const_gui_pref
{
public:
    // レイアウトプリセット関連のディレクトリ
    static inline const char* DefaultPresetDirectory = "config/gui/";
    
    // デフォルトプリセットファイル名（テンプレート）
    static inline const char* LayoutMakeModeDefaultFile = "layout_makemode.ini";
    static inline const char* LayoutDebugModeDefaultFile = "layout_debugmode.ini";
    
    // 設定ファイル名
    static inline const char* LayoutConfigFile = "layout_config.json";
    
    // プリセット名（JSON保存用）
    static inline const char* PresetNameMakeMode = "makemode";
    static inline const char* PresetNameDebugMode = "debugmode";
};
