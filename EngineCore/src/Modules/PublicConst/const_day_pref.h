#pragma once

class const_day_pref
{
public:
    // TODO: EngineCoreからの相対パスになっているので、Gameからの相対パスに変更する

	// Daysディレクトリのパス
	static inline const char* DaysDirectoryPath = "../Game/assets/Days/";
	static inline const char* DaysDirectoryPathGame = "../Game/assets/Days";
	
	// TmpDay関連
	static inline const char* TmpDayFileName = "new_day_scene_tmp.json";
	static inline const char* TmpDayPath = "../Game/assets/Tmp/new_day_scene_tmp.json";
	
	// ファイル拡張子
	static inline const char* DayFileExtension = ".json";
	
	// ダイアログ関連
	static inline const char* SaveDialogTitle = "Save Day As";
	static inline const char* SaveDialogPrompt = "Please enter the day name:";
};
