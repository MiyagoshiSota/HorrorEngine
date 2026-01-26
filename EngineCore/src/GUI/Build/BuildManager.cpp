#include "BuildManager.h"
#include "Modules/PublicConst/ConstBuildPref.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <iomanip>
#include <tlhelp32.h>
#include <cstring>

// ログファイルに書き込むヘルパー関数
static void WriteLog(const std::filesystem::path& logPath, const std::string& message)
{
    std::ofstream logFile(logPath, std::ios::app);
    if (logFile.is_open())
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        logFile << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;
        logFile.flush();
    }
}

bool BuildManager::ExecuteBuild(const std::vector<std::string>& sceneList, const std::string& buildConfig)
{
    if (sceneList.empty())
    {
        m_lastError = "No scenes selected for build";
        return false;
    }
    
    if (m_isBuilding)
    {
        m_lastError = "Build is already in progress";
        return false;
    }

    m_isBuilding = true;
    m_lastError.clear();

    // プロジェクトルートディレクトリを取得（ログファイルのパスを決定するため先に取得）
    std::filesystem::path currentDir = std::filesystem::current_path();
    std::filesystem::path solutionPath = currentDir / ConstBuildPref::kSolutionFileName;
    
    // 現在のディレクトリにない場合は、親ディレクトリを確認
    if (!std::filesystem::exists(solutionPath))
    {
        solutionPath = currentDir.parent_path() / ConstBuildPref::kSolutionFileName;
    }
    
    if (!std::filesystem::exists(solutionPath))
    {
        m_lastError = "Solution file not found: " + solutionPath.string();
        m_isBuilding = false;
        return false;
    }
    
    // プロジェクトルートディレクトリを取得
    std::filesystem::path projectRoot = solutionPath.parent_path();
    
    // ビルドログファイルのパス
    std::filesystem::path buildLogPath = projectRoot / ConstBuildPref::kBuildLogFileName;
    
    // ログファイルをクリアして開始ログを書き込む
    std::ofstream logFile(buildLogPath, std::ios::trunc);
    if (logFile.is_open())
    {
        logFile << "=== Build Process Started ===" << std::endl;
        logFile << "Project Root: " << projectRoot.string() << std::endl;
        logFile << "Solution Path: " << solutionPath.string() << std::endl;
        logFile << "Build Config: " << buildConfig << std::endl;
        logFile << "Scenes Count: " << sceneList.size() << std::endl;
        logFile.flush();
        logFile.close();
    }

    WriteLog(buildLogPath, "[STEP 1] Starting build process...");

    // MSBuildのパスを取得
    WriteLog(buildLogPath, "[STEP 2] Finding MSBuild path...");
    std::string msbuildPath = FindMSBuildPath();
    if (msbuildPath.empty())
    {
        WriteLog(buildLogPath, "[ERROR] MSBuild not found!");
        m_lastError = "MSBuild not found. Please install Visual Studio.";
        m_isBuilding = false;
        return false;
    }
    WriteLog(buildLogPath, "[STEP 2] MSBuild found: " + msbuildPath);

    // 実行中のGame.exeプロセスをチェック
    if (IsGameProcessRunning())
    {
        WriteLog(buildLogPath, "[WARNING] Game.exe is currently running. The build may fail with LNK1104 error if Game.exe is locked.");
        WriteLog(buildLogPath, "[WARNING] Please close Game.exe manually before building to avoid linker errors.");
    }

    // ビルド出力ディレクトリを作成
    WriteLog(buildLogPath, "[STEP 3] Creating build output directory...");
    if (!CreateBuildDirectory())
    {
        WriteLog(buildLogPath, "[ERROR] Failed to create build output directory!");
        m_lastError = "Failed to create build output directory";
        m_isBuilding = false;
        return false;
    }
    WriteLog(buildLogPath, "[STEP 3] Build output directory created successfully");

    // MSBuildコマンドを構築
    WriteLog(buildLogPath, "[STEP 4] Building MSBuild command...");
    
    // Gameプロジェクトの出力先をBuilds\Release\に設定（実行中のGame.exeをロックしないため）
    std::filesystem::path gameOutDir = projectRoot / ConstBuildPref::kBuildOutputDirectory / buildConfig;
    std::string gameOutDirStr = gameOutDir.string();
    // パスの区切り文字をバックスラッシュに統一（MSBuild用）
    std::replace(gameOutDirStr.begin(), gameOutDirStr.end(), '/', '\\');
    if (gameOutDirStr.back() != '\\')
    {
        gameOutDirStr += "\\";
    }
    
    std::stringstream cmdStream;
    cmdStream << "\"" << msbuildPath << "\" ";
    cmdStream << "\"" << solutionPath.string() << "\" ";
    cmdStream << "/p:Configuration=" << buildConfig << " ";
    cmdStream << "/p:Platform=" << ConstBuildPref::kMSBuildPlatform << " ";
    // GamePreprocessorDefinitionsプロパティを使用して、既存の定義を保持しつつBUILD_STANDALONEを追加
    cmdStream << "/p:GamePreprocessorDefinitions=\"" << ConstBuildPref::kMSBuildPreprocessorDefinition << "\" ";
    cmdStream << "/p:GameOutDir=" << gameOutDirStr << " "; // GameプロジェクトのOutDirを上書き（引用符なし）
    cmdStream << "/t:Build ";
    cmdStream << "/v:detailed "; // 詳細な出力を取得するため詳細度を上げる
    cmdStream << "/nologo";

    std::string cmd = cmdStream.str();
    WriteLog(buildLogPath, "[STEP 4] MSBuild command: " + cmd);
    
    // CreateProcessAの第2引数は書き換え可能なバッファが必要
    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    // MSBuildを実行
    WriteLog(buildLogPath, "[STEP 5] Preparing MSBuild process...");
    STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);
    
    // ビルドログファイルを作成（MSBuildの出力をリダイレクト）
    WriteLog(buildLogPath, "[STEP 5] Creating output file handle for MSBuild...");
    
    // セキュリティ属性を設定してハンドルを継承可能にする
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; // 子プロセスに継承可能にする
    sa.lpSecurityDescriptor = nullptr;
    
    HANDLE hOutputFile = CreateFileA(
        buildLogPath.string().c_str(),
        GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        &sa, // セキュリティ属性を指定
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    // ファイルが作成できた場合は、標準出力とエラー出力をリダイレクト
    if (hOutputFile != INVALID_HANDLE_VALUE)
    {
        WriteLog(buildLogPath, "[STEP 5] Output file handle created successfully");
        // ファイルポインタを末尾に移動（追記モード）
        SetFilePointer(hOutputFile, 0, nullptr, FILE_END);
        
        // MSBuildの出力をリダイレクトする前に区切り線を追加
        std::string separator = "\n=== MSBuild Output ===\n";
        DWORD written = 0;
        WriteFile(hOutputFile, separator.c_str(), static_cast<DWORD>(separator.length()), &written, nullptr);
        FlushFileBuffers(hOutputFile);
        
        // 標準入出力ハンドルを設定
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hOutputFile;
        si.hStdError = hOutputFile;
        // 標準入力ハンドルを設定（NULデバイスを使用）
        HANDLE hNullInput = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
        if (hNullInput != INVALID_HANDLE_VALUE)
        {
            si.hStdInput = hNullInput;
        }
        else
        {
            // NULデバイスが作成できない場合は、現在の標準入力を使用
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        }
    }
    else
    {
        DWORD fileError = ::GetLastError();
        WriteLog(buildLogPath, "[ERROR] Failed to create output file handle. Error code: " + std::to_string(fileError));
        // ファイルハンドルが作成できない場合は、リダイレクトしない
        si.dwFlags = 0;
    }

    // 作業ディレクトリをプロジェクトルートに設定
    std::string workingDir = projectRoot.string();
    WriteLog(buildLogPath, "[STEP 6] Working directory: " + workingDir);
    std::vector<char> workingDirBuffer(workingDir.begin(), workingDir.end());
    workingDirBuffer.push_back('\0');

    WriteLog(buildLogPath, "[STEP 7] Starting MSBuild process...");
    WriteLog(buildLogPath, "[DEBUG] hStdOutput handle: " + std::to_string(reinterpret_cast<DWORD_PTR>(si.hStdOutput)));
    WriteLog(buildLogPath, "[DEBUG] hStdError handle: " + std::to_string(reinterpret_cast<DWORD_PTR>(si.hStdError)));
    WriteLog(buildLogPath, "[DEBUG] hStdInput handle: " + std::to_string(reinterpret_cast<DWORD_PTR>(si.hStdInput)));
    WriteLog(buildLogPath, "[DEBUG] dwFlags: " + std::to_string(si.dwFlags));
    
    bool success = CreateProcessA(
        nullptr,
        cmdBuffer.data(),
        nullptr,
        nullptr,
        TRUE, // bInheritHandles = TRUE（ハンドルを継承）
        CREATE_NO_WINDOW, // コンソールウィンドウを作成しない
        nullptr,
        workingDirBuffer.data(), // 作業ディレクトリをプロジェクトルートに設定
        &si,
        &pi
    );

    if (!success)
    {
        DWORD error = ::GetLastError();
        WriteLog(buildLogPath, "[ERROR] Failed to start MSBuild process. Error code: " + std::to_string(error));
        m_lastError = "Failed to start MSBuild. Error code: " + std::to_string(error);
        if (hOutputFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hOutputFile);
        }
        m_isBuilding = false;
        return false;
    }
    WriteLog(buildLogPath, "[STEP 7] MSBuild process started successfully. Process ID: " + std::to_string(pi.dwProcessId));

    // プロセスの終了を待つ
    WriteLog(buildLogPath, "[STEP 8] Waiting for MSBuild process to complete...");
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    // MSBuildの出力を確実に取得するため、プロセス終了後にフラッシュ
    if (hOutputFile != INVALID_HANDLE_VALUE)
    {
        // 区切り線を追加
        std::string separator = "\n=== MSBuild Process Completed ===\n";
        DWORD written = 0;
        WriteFile(hOutputFile, separator.c_str(), static_cast<DWORD>(separator.length()), &written, nullptr);
        FlushFileBuffers(hOutputFile);
        
        // ファイルサイズを確認
        LARGE_INTEGER fileSize = {};
        if (GetFileSizeEx(hOutputFile, &fileSize))
        {
            WriteLog(buildLogPath, "[DEBUG] Output file size after MSBuild: " + std::to_string(fileSize.QuadPart) + " bytes");
        }
    }
    
    WriteLog(buildLogPath, "[STEP 8] MSBuild process completed. Exit code: " + std::to_string(exitCode));

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    // 標準入力ハンドルを閉じる（NULデバイスの場合）
    if (si.hStdInput != INVALID_HANDLE_VALUE && si.hStdInput != GetStdHandle(STD_INPUT_HANDLE))
    {
        CloseHandle(si.hStdInput);
    }
    
    if (hOutputFile != INVALID_HANDLE_VALUE)
    {
        // バッファをフラッシュしてからファイルを閉じる
        WriteLog(buildLogPath, "[STEP 9] Flushing and closing output file handle...");
        FlushFileBuffers(hOutputFile);
        CloseHandle(hOutputFile);
        WriteLog(buildLogPath, "[STEP 9] Output file handle closed");
    }

    if (exitCode != 0)
    {
        WriteLog(buildLogPath, "[ERROR] Build failed with exit code: " + std::to_string(exitCode));
        m_lastError = "Build failed. Exit code: " + std::to_string(exitCode) + ". Check " + std::string(ConstBuildPref::kBuildLogFileName) + " for details.";
        m_isBuilding = false;
        return false;
    }
    
    WriteLog(buildLogPath, "[STEP 9] Build succeeded!");

    // ビルド出力ディレクトリのパス（プロジェクトルートからの相対パス）
    std::filesystem::path buildOutputDir = projectRoot / ConstBuildPref::kBuildOutputDirectory;
    std::filesystem::path buildConfigDir = buildOutputDir / buildConfig;

    // 実行ファイルをコピー
    WriteLog(buildLogPath, "[STEP 11] Copying executable...");
    if (!CopyExecutable(buildConfigDir, buildConfig, projectRoot))
    {
        WriteLog(buildLogPath, "[ERROR] Failed to copy executable: " + m_lastError);
        m_isBuilding = false;
        return false;
    }
    WriteLog(buildLogPath, "[STEP 11] Executable copied successfully");

    // リソースファイルをコピー（シーンリストを渡す）
    WriteLog(buildLogPath, "[STEP 12] Copying resource files...");
    if (!CopyResourceFiles(buildConfigDir, sceneList, projectRoot))
    {
        WriteLog(buildLogPath, "[ERROR] Failed to copy resource files: " + m_lastError);
        m_isBuilding = false;
        return false;
    }
    WriteLog(buildLogPath, "[STEP 12] Resource files copied successfully");

    WriteLog(buildLogPath, "=== Build Process Completed Successfully ===");
    m_isBuilding = false;
    return true;
}

std::string BuildManager::FindMSBuildPath()
{
    // vswhereを使用してMSBuildのパスを取得
    std::string vswherePath = "\"" + std::string(ConstBuildPref::kVswherePath) + "\"";
    
    if (!std::filesystem::exists(ConstBuildPref::kVswherePath))
    {
        // vswhereが見つからない場合は、一般的なパスを試す
        const std::vector<std::string> commonPaths = {
            ConstBuildPref::kMSBuildPath2022Community,
            ConstBuildPref::kMSBuildPath2022Professional,
            ConstBuildPref::kMSBuildPath2022Enterprise,
            ConstBuildPref::kMSBuildPath2019Community,
            ConstBuildPref::kMSBuildPath2019Professional,
            ConstBuildPref::kMSBuildPath2019Enterprise,
        };

        for (const auto& path : commonPaths)
        {
            if (std::filesystem::exists(path))
            {
                return path;
            }
        }

        return "";
    }

    // vswhereを使用してMSBuildのパスを取得
    std::string cmd = vswherePath + " -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe";
    
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe)
    {
        return "";
    }

    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    _pclose(pipe);

    // 改行を除去
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());

    if (std::filesystem::exists(result))
    {
        return result;
    }

    return "";
}

bool BuildManager::CreateBuildDirectory()
{
    try
    {
        // プロジェクトルートを取得
        std::filesystem::path currentDir = std::filesystem::current_path();
        std::filesystem::path solutionPath = currentDir / ConstBuildPref::kSolutionFileName;
        if (!std::filesystem::exists(solutionPath))
        {
            solutionPath = currentDir.parent_path() / ConstBuildPref::kSolutionFileName;
        }
        std::filesystem::path projectRoot = solutionPath.parent_path();
        
        std::filesystem::path buildDir = projectRoot / ConstBuildPref::kBuildOutputDirectory;
        std::filesystem::create_directories(buildDir);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool BuildManager::IsGameProcessRunning()
{
    try
    {
        // プロセススナップショットを作成
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        PROCESSENTRY32 pe32 = {};
        pe32.dwSize = sizeof(PROCESSENTRY32);

        // 最初のプロセスを取得
        if (!Process32First(hSnapshot, &pe32))
        {
            CloseHandle(hSnapshot);
            return false;
        }

        do
        {
            // プロセス名がGame.exeか確認
            if (_stricmp(pe32.szExeFile, ConstBuildPref::kExecutableFileName) == 0)
            {
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32Next(hSnapshot, &pe32));

        CloseHandle(hSnapshot);
        return false;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool BuildManager::CopyResourceFiles(const std::filesystem::path& buildOutputDir, const std::vector<std::string>& sceneList, const std::filesystem::path& projectRoot)
{
    try
    {
        // Assetsディレクトリ全体をコピー
        std::filesystem::path assetsSource = projectRoot / ConstBuildPref::kGameDirectoryName / ConstBuildPref::kAssetsDirectoryName;
        std::filesystem::path assetsDest = buildOutputDir / ConstBuildPref::kAssetsDirectoryName;
        
        if (std::filesystem::exists(assetsSource))
        {
            // 既存のAssetsディレクトリを削除（上書きのため）
            if (std::filesystem::exists(assetsDest))
            {
                std::filesystem::remove_all(assetsDest);
            }
            
            // Assetsディレクトリ全体を再帰的にコピー
            std::filesystem::create_directories(assetsDest.parent_path());
            std::filesystem::copy(assetsSource, assetsDest, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        }
        else
        {
            m_lastError = "Assets directory not found: " + assetsSource.string();
            return false;
        }

        // Configディレクトリもコピー
        std::filesystem::path configSource = projectRoot / ConstBuildPref::kGameDirectoryName / ConstBuildPref::kConfigDirectoryName;
        std::filesystem::path configDest = buildOutputDir / ConstBuildPref::kConfigDirectoryName;

        if (std::filesystem::exists(configSource))
        {
            if (std::filesystem::exists(configDest))
            {
                std::filesystem::remove_all(configDest);
            }
            std::filesystem::create_directories(configDest.parent_path());
            std::filesystem::copy(configSource, configDest, std::filesystem::copy_options::recursive);
        }

        return true;
    }
    catch (const std::exception& e)
    {
        m_lastError = "Failed to copy resources: " + std::string(e.what());
        return false;
    }
}

bool BuildManager::CopyExecutable(const std::filesystem::path& buildOutputDir, const std::string& buildConfig, const std::filesystem::path& projectRoot)
{
    try
    {
        // MSBuildがBuilds\Release\Game.exeに直接出力するようになったため、
        // 実行ファイルが既に正しい場所にあることを確認するだけ
        std::filesystem::path exePath = buildOutputDir / ConstBuildPref::kExecutableFileName;
        
        if (!std::filesystem::exists(exePath))
        {
            m_lastError = "Executable not found: " + exePath.string();
            return false;
        }

        return true;
    }
    catch (const std::exception& e)
    {
        m_lastError = "Failed to verify executable: " + std::string(e.what());
        return false;
    }
}
