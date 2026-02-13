#include "LayoutPresetManager.h"
#include "imgui.h"
#include "Renderer/Engine.h"
#include "GUI/Windows/DrawGameObjectWindow.h"
#include "GUI/Windows/DrawModeWindow.h"
#include "GUI/Windows/DrawPostProcessPresetWindow.h"
#include "GUI/Windows/DrawModelsWindow.h"
#include "GUI/Windows/DrawTaskManagerWindow.h"
#include "GUI/Windows/DrawWorkManagerWindow.h"
#include "GUI/Windows/DrawPlayerWindow.h"
#include "GUI/Windows/DrawDayWindow.h"
#include "Modules/PublicConst/ConstGuiPref.h"
#include <fstream>
#include <iostream>
#include <filesystem>

void LayoutPresetManager::ApplyPreset(LayoutPresetType presetType)
{
    if (!g_Engine)
        return;

    // ウィンドウの表示/非表示を設定
    SetWindowVisibility(presetType);

    // 現在のプリセットを更新
    m_currentPreset = presetType;

    // 現在のプリセットを保存
    SaveCurrentPreset(presetType);
}

void LayoutPresetManager::LoadPresetLayout(LayoutPresetType presetType)
{
    // デフォルトプリセットファイルからGame/imgui.iniにコピー
    std::string defaultPath = GetDefaultPresetFilePath(presetType);
    std::filesystem::path defaultFilePath(defaultPath);

    if (std::filesystem::exists(defaultFilePath))
    {
        // デフォルトプリセットファイルが存在する場合は、Game/imgui.iniにコピー
        try
        {
            std::filesystem::copy_file(defaultFilePath, m_imguiIniPath, std::filesystem::copy_options::overwrite_existing);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to copy preset file to imgui.ini: " << e.what() << std::endl;
        }
    }

    // Game/imgui.iniを読み込んでレイアウトを適用
    // 注意: この関数はNewFrame()の前に呼ぶ必要がある
    ImGui::LoadIniSettingsFromDisk(m_imguiIniPath.c_str());
}

void LayoutPresetManager::SetWindowVisibility(LayoutPresetType presetType)
{
    if (!g_Engine)
        return;

    auto& windows = g_Engine->GetDrawWindows();
    
    // モードウィンドウは固定UIとして別途管理されている
    std::shared_ptr<IDrawWindow> modeWindow = g_Engine->GetModeWindow();
    std::shared_ptr<DrawModeWindow> modeWindowTyped = std::dynamic_pointer_cast<DrawModeWindow>(modeWindow);

    // 各ウィンドウを検索
    std::shared_ptr<DrawGameObjectWindow> gameObjectWindow;
    std::shared_ptr<DrawPostProcessPresetWindow> postProcessWindow;
    std::shared_ptr<DrawModelsWindow> modelsWindow;
    std::shared_ptr<DrawTaskManagerWindow> taskManagerWindow;
    std::shared_ptr<DrawWorkManagerWindow> workManagerWindow;
    std::shared_ptr<DrawPlayerWindow> playerWindow;
    std::shared_ptr<DrawDayWindow> dayWindow;

    for (auto& window : windows)
    {
        if (!gameObjectWindow) gameObjectWindow = std::dynamic_pointer_cast<DrawGameObjectWindow>(window);
        if (!postProcessWindow) postProcessWindow = std::dynamic_pointer_cast<DrawPostProcessPresetWindow>(window);
        if (!modelsWindow) modelsWindow = std::dynamic_pointer_cast<DrawModelsWindow>(window);
        if (!taskManagerWindow) taskManagerWindow = std::dynamic_pointer_cast<DrawTaskManagerWindow>(window);
        if (!workManagerWindow) workManagerWindow = std::dynamic_pointer_cast<DrawWorkManagerWindow>(window);
        if (!playerWindow) playerWindow = std::dynamic_pointer_cast<DrawPlayerWindow>(window);
        if (!dayWindow) dayWindow = std::dynamic_pointer_cast<DrawDayWindow>(window);
    }

    // プリセットに応じて表示/非表示を設定
    switch (presetType)
    {
    case LayoutPresetType::MakeMode:
        // MakeMode: Day, Work Manager, Task Manager, Game Object Window, Models Window, Player Window を表示
        // Mode Window は常に表示（固定UI）
        // Post Process Preset Window を非表示
        if (dayWindow) dayWindow->SetVisible(true);
        if (workManagerWindow) workManagerWindow->SetVisible(true);
        if (taskManagerWindow) taskManagerWindow->SetVisible(true);
        if (gameObjectWindow) gameObjectWindow->SetVisible(true);
        if (modelsWindow) modelsWindow->SetVisible(true);
        if (playerWindow) playerWindow->SetVisible(true);
        if (modeWindowTyped) modeWindowTyped->SetVisible(true);
        if (postProcessWindow) postProcessWindow->SetVisible(false);
        break;

    case LayoutPresetType::DebugMode:
        // DebugMode: Post Process Preset Window のみ表示
        // Mode Window は常に表示（固定UI）
        // Day Window はモーダルダイアログ表示のために常に表示
        if (dayWindow) dayWindow->SetVisible(true);
        if (workManagerWindow) workManagerWindow->SetVisible(false);
        if (taskManagerWindow) taskManagerWindow->SetVisible(false);
        if (gameObjectWindow) gameObjectWindow->SetVisible(false);
        if (modelsWindow) modelsWindow->SetVisible(false);
        if (playerWindow) playerWindow->SetVisible(false);
        if (modeWindowTyped) modeWindowTyped->SetVisible(true);
        if (postProcessWindow) postProcessWindow->SetVisible(true);
        break;
    }
}


std::string LayoutPresetManager::GetDefaultPresetFilePath(LayoutPresetType presetType) const
{
    const char* filename = nullptr;
    switch (presetType)
    {
    case LayoutPresetType::MakeMode:
        filename = ConstGuiPref::kLayoutMakeModeDefaultFile;
        break;
    case LayoutPresetType::DebugMode:
        filename = ConstGuiPref::kLayoutDebugModeDefaultFile;
        break;
    }
    return std::string(ConstGuiPref::kDefaultPresetDirectory) + filename;
}

std::string LayoutPresetManager::GetConfigFilePath() const
{
    return m_configDir + ConstGuiPref::kLayoutConfigFile;
}

LayoutPresetType LayoutPresetManager::LoadLastPreset()
{
    std::string configPath = GetConfigFilePath();
    std::filesystem::path filePath(configPath);

    // ファイルが存在しない場合はデフォルト（MakeMode）を返す
    if (!std::filesystem::exists(filePath))
    {
        return LayoutPresetType::MakeMode;
    }

    try
    {
        std::ifstream file(configPath);
        if (!file.is_open())
        {
            return LayoutPresetType::MakeMode;
        }

        json config;
        file >> config;

        if (config.contains("last_preset"))
        {
            std::string presetName = config["last_preset"];
            if (presetName == ConstGuiPref::kPresetNameMakeMode)
            {
                return LayoutPresetType::MakeMode;
            }
            else if (presetName == ConstGuiPref::kPresetNameDebugMode)
            {
                return LayoutPresetType::DebugMode;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load layout config: " << e.what() << std::endl;
    }

    return LayoutPresetType::MakeMode;
}

void LayoutPresetManager::SaveCurrentPreset(LayoutPresetType presetType)
{
    std::string configPath = GetConfigFilePath();
    std::filesystem::path filePath(configPath);

    // ディレクトリが存在しない場合は作成
    if (filePath.has_parent_path())
    {
        std::filesystem::create_directories(filePath.parent_path());
    }

    try
    {
        json config;
        const char* presetName = nullptr;
        switch (presetType)
        {
        case LayoutPresetType::MakeMode:
            presetName = ConstGuiPref::kPresetNameMakeMode;
            break;
        case LayoutPresetType::DebugMode:
            presetName = ConstGuiPref::kPresetNameDebugMode;
            break;
        }
        config["last_preset"] = presetName;

        std::ofstream file(configPath);
        if (file.is_open())
        {
            file << config.dump(4); // インデント付きで保存
            file.close();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to save layout config: " << e.what() << std::endl;
    }
}
