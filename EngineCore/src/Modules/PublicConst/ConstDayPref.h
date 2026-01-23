#pragma once

class ConstDayPref
{
public:
    // TODO: EngineCoreからの相対パスになっているので、Gameからの相対パスに変更する

	// Daysディレクトリのパス
	static inline const char* kDaysDirectoryPath = "../Game/Assets/Days/";
	static inline const char* kDaysDirectoryPathGame = "../Game/Assets/Days";
	
	// TmpDay関連
	static inline const char* kTmpDayFileName = "new_day_scene_tmp.json";
	static inline const char* kTmpDayPath = "../Game/Assets/Tmp/new_day_scene_tmp.json";
	
	// ファイル拡張子
	static inline const char* kDayFileExtension = ".json";
	
	// ダイアログ関連
	static inline const char* kSaveDialogTitle = "Save Day As";
	static inline const char* kSaveDialogPrompt = "Please enter the day name:";
};
