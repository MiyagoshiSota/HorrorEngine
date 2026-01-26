#pragma once

class ConstBuildPref
{
public:
	// ビルド先ディレクトリ（プロジェクトルートからの相対パス）
	static inline const char* kBuildOutputDirectory = "Builds/";
	
	// ビルド設定JSONファイルのパス（Game/Assetsからの相対パス）
	static inline const char* kBuildConfigJsonPath = "Assets/build_config.json";
	
	// ビルド設定JSONのキー
	static inline const char* kSceneListKey = "scene_list"; // シーンリスト
	static inline const char* kBuildConfigKey = "build_config";
	
	// ソリューションファイル（プロジェクトルートからの相対パス）
	static inline const char* kSolutionFileName = "HorrorEngine.sln";
	
	// ビルドログファイル（プロジェクトルートからの相対パス）
	static inline const char* kBuildLogFileName = "build_log.txt";
	
	// MSBuild設定
	static inline const char* kMSBuildPreprocessorDefinition = "BUILD_STANDALONE";
	static inline const char* kMSBuildPlatform = "x64";
	static inline const char* kMSBuildVerbosity = "m"; // minimal
	
	// vswhereパス
	static inline const char* kVswherePath = R"(C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe)";
	
	// MSBuildの一般的なパス（vswhereが見つからない場合のフォールバック）
	static inline const char* kMSBuildPath2022Community = R"(C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe)";
	static inline const char* kMSBuildPath2022Professional = R"(C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe)";
	static inline const char* kMSBuildPath2022Enterprise = R"(C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe)";
	static inline const char* kMSBuildPath2019Community = R"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe)";
	static inline const char* kMSBuildPath2019Professional = R"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe)";
	static inline const char* kMSBuildPath2019Enterprise = R"(C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe)";
	
	// ディレクトリ名
	static inline const char* kGameDirectoryName = "Game";
	static inline const char* kAssetsDirectoryName = "Assets";
	static inline const char* kDaysDirectoryName = "Days";
	static inline const char* kConfigDirectoryName = "Config";
	
	// 実行ファイル
	static inline const char* kExecutableFileName = "Game.exe";
	static inline const char* kExecutableSourceDirectory = "Game/x64";
	
	// Assetsファイル名
	static inline const char* kModelsJsonFileName = "models.json";
	static inline const char* kSceneJsonFileName = "scene.json";
	static inline const char* kPSODefinitionsJsonFileName = "pso_definitions.json";
	static inline const char* kPostProcessPresetsJsonFileName = "postprocess_presets.json";
};
