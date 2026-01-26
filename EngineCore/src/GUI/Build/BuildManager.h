#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <Windows.h>
#include <process.h>

class BuildManager
{
public:
    static BuildManager& GetInstance()
    {
        static BuildManager instance;
        return instance;
    }

    // ビルドを実行
    bool ExecuteBuild(const std::vector<std::string>& sceneList, const std::string& buildConfig = "Release");

    // ビルド状態の取得
    bool IsBuilding() const { return m_isBuilding; }
    std::string GetLastError() const { return m_lastError; }

private:
    BuildManager() = default;
    ~BuildManager() = default;
    BuildManager(const BuildManager&) = delete;
    BuildManager& operator=(const BuildManager&) = delete;

    // MSBuildのパスを取得
    std::string FindMSBuildPath();

    // ビルド出力ディレクトリを作成
    bool CreateBuildDirectory();

    // 実行中のGame.exeプロセスを検出（終了はしない）
    bool IsGameProcessRunning();

    // リソースファイルをコピー
    bool CopyResourceFiles(const std::filesystem::path& buildOutputDir, const std::vector<std::string>& sceneList, const std::filesystem::path& projectRoot);

    // シェーダーファイルをコピー
    bool CopyShaderFiles(const std::filesystem::path& buildOutputDir, const std::string& buildConfig, const std::filesystem::path& projectRoot);

    // 実行ファイルをコピー
    bool CopyExecutable(const std::filesystem::path& buildOutputDir, const std::string& buildConfig, const std::filesystem::path& projectRoot);

private:
    bool m_isBuilding = false;
    std::string m_lastError;
};
