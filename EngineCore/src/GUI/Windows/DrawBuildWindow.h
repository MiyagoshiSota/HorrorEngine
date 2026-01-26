#pragma once
#include "GUI/Core/IDrawWindow.h"
#include "GUI/Build/BuildManager.h"
#include "imgui.h"
#include "Core/App.h"
#include "Scene/SceneManager.h"
#include "Modules/PublicConst/ConstBuildPref.h"
#include "Modules/PublicConst/ConstDayPref.h"
#include <string>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include <algorithm>

class DrawBuildWindow : public IDrawWindow
{
public:
    void draw() override
    {
        if (!ImGui::Begin("Build Window", &m_isVisible))
        {
            ImGui::End();
            return;
        }

        ImGui::Text("Scenes in Build:");
        ImGui::Separator();

        // 2カラムレイアウト
        ImGui::Columns(2, "BuildScenes", true);
        
        // 左側: 利用可能なシーンリスト
        ImGui::Text("Available Scenes");
        ImGui::Separator();
        
        // プロジェクトルートを取得
        std::filesystem::path projectRoot = GetProjectRoot();
        std::filesystem::path daysDir = projectRoot / ConstBuildPref::kGameDirectoryName / ConstBuildPref::kAssetsDirectoryName / ConstBuildPref::kDaysDirectoryName;
        if (std::filesystem::exists(daysDir))
        {
            if (ImGui::BeginChild("AvailableScenes", ImVec2(0, 300), true))
            {
                for (const auto& entry : std::filesystem::directory_iterator(daysDir))
                {
                    if (entry.is_regular_file() && entry.path().extension() == ConstDayPref::kDayFileExtension)
                    {
                        std::string sceneName = entry.path().stem().string();
                        // プロジェクトルートからの相対パスとして保存（Game/Assets/Days/...）
                        std::filesystem::path relativePath = std::filesystem::relative(entry.path(), projectRoot);
                        std::string scenePath = relativePath.string();
                        // パス区切りを統一（Windowsではバックスラッシュ、std::filesystemは自動変換）
                        std::replace(scenePath.begin(), scenePath.end(), '\\', '/');
                        
                        // 既にビルドリストに含まれているかチェック
                        bool isInBuildList = std::find(m_buildSceneList.begin(), m_buildSceneList.end(), scenePath) != m_buildSceneList.end();
                        
                        if (ImGui::Selectable(sceneName.c_str(), false))
                        {
                            // クリックで追加/削除をトグル
                            if (isInBuildList)
                            {
                                m_buildSceneList.erase(std::remove(m_buildSceneList.begin(), m_buildSceneList.end(), scenePath), m_buildSceneList.end());
                            }
                            else
                            {
                                m_buildSceneList.push_back(scenePath);
                            }
                        }
                        
                        // ビルドリストに含まれている場合は視覚的に表示
                        if (isInBuildList)
                        {
                            ImGui::SameLine();
                            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[In Build]");
                        }
                    }
                }
            }
            ImGui::EndChild();
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Days directory not found!");
        }

        ImGui::NextColumn();

        // 右側: ビルドに含まれるシーンリスト
        ImGui::Text("Scenes in Build");
        ImGui::Separator();
        
        if (ImGui::BeginChild("BuildSceneList", ImVec2(0, 300), true))
        {
            if (m_buildSceneList.empty())
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No scenes added");
            }
            else
            {
                for (size_t i = 0; i < m_buildSceneList.size(); ++i)
                {
                    std::string scenePath = m_buildSceneList[i];
                    std::string sceneName = std::filesystem::path(scenePath).stem().string();
                    
                    ImGui::PushID(static_cast<int>(i));
                    
                    // 上矢印ボタン
                    if (i > 0)
                    {
                        if (ImGui::Button("^"))
                        {
                            std::swap(m_buildSceneList[i], m_buildSceneList[i - 1]);
                        }
                        ImGui::SameLine();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
                        ImGui::Button("^");
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                    }
                    
                    // 下矢印ボタン
                    if (i < m_buildSceneList.size() - 1)
                    {
                        if (ImGui::Button("v"))
                        {
                            std::swap(m_buildSceneList[i], m_buildSceneList[i + 1]);
                        }
                        ImGui::SameLine();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
                        ImGui::Button("v");
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                    }
                    
                    // シーン名
                    ImGui::Text("%d. %s", static_cast<int>(i), sceneName.c_str());
                    ImGui::SameLine();
                    
                    // 削除ボタン
                    if (ImGui::Button("Remove"))
                    {
                        m_buildSceneList.erase(m_buildSceneList.begin() + i);
                        ImGui::PopID();
                        break; // ループを抜ける
                    }
                    
                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();
        
        ImGui::Columns(1);

        ImGui::Separator();

        // ビルドボタン
        bool canBuild = !m_buildSceneList.empty() && !m_isBuilding;
        if (!canBuild)
        {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Build", ImVec2(-1, 0)))
        {
            SaveBuildConfig();
            StartBuild();
        }

        if (!canBuild)
        {
            ImGui::EndDisabled();
        }

        // ビルド中の表示
        if (m_isBuilding || BuildManager::GetInstance().IsBuilding())
        {
            m_isBuilding = true; // BuildManagerの状態と同期
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Building...");
        }
        else
        {
            m_isBuilding = false;
        }

        // ビルド結果の表示
        if (!m_buildResultMessage.empty())
        {
            ImVec4 color = m_buildSucceeded ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            ImGui::TextColored(color, m_buildResultMessage.c_str());
        }

        ImGui::End();
    }

private:
    void SaveBuildConfig()
    {
        if (m_buildSceneList.empty())
        {
            return;
        }

        // プロジェクトルートを取得
        std::filesystem::path projectRoot = GetProjectRoot();
        // Game/Assets/build_config.jsonに書き込む
        std::filesystem::path configPath = projectRoot / ConstBuildPref::kGameDirectoryName / ConstBuildPref::kAssetsDirectoryName / "build_config.json";
        
        // Assetsディレクトリが存在しない場合は作成
        std::filesystem::path assetsDir = configPath.parent_path();
        if (!std::filesystem::exists(assetsDir))
        {
            std::filesystem::create_directories(assetsDir);
        }

        nlohmann::json configJson;
        
        // シーンリストを保存
        configJson[ConstBuildPref::kSceneListKey] = m_buildSceneList;
        configJson[ConstBuildPref::kBuildConfigKey] = "Release"; // デフォルトはRelease

        std::ofstream ofs(configPath);
        if (ofs.is_open())
        {
            ofs << configJson.dump(4);
            ofs.close();
        }
    }

    void StartBuild()
    {
        if (m_buildSceneList.empty())
        {
            return;
        }

        if (m_isBuilding)
        {
            return;
        }

        m_isBuilding = true;
        m_buildResultMessage.clear();
        m_buildSucceeded = false;

        // ビルドに含まれるシーンリストをコピー（スレッドセーフのため）
        std::vector<std::string> sceneList = m_buildSceneList;

        // 非同期でビルドを実行
        std::thread buildThread([this, sceneList]() {
            auto& buildManager = BuildManager::GetInstance();
            bool success = buildManager.ExecuteBuild(sceneList, "Release");
            
            m_isBuilding = false;
            m_buildSucceeded = success;
            if (success)
            {
                m_buildResultMessage = "Build succeeded! Output: " + std::string(ConstBuildPref::kBuildOutputDirectory);
            }
            else
            {
                m_buildResultMessage = "Build failed: " + buildManager.GetLastError();
            }
        });
        buildThread.detach();
    }

    // プロジェクトルートを取得するヘルパー関数
    std::filesystem::path GetProjectRoot()
    {
        std::filesystem::path currentDir = std::filesystem::current_path();
        std::filesystem::path solutionPath = currentDir / ConstBuildPref::kSolutionFileName;
        
        // 現在のディレクトリにない場合は、親ディレクトリを確認
        if (!std::filesystem::exists(solutionPath))
        {
            solutionPath = currentDir.parent_path() / ConstBuildPref::kSolutionFileName;
        }
        
        if (std::filesystem::exists(solutionPath))
        {
            return solutionPath.parent_path();
        }
        
        // 見つからない場合は現在のディレクトリを返す
        return currentDir;
    }

private:
    std::vector<std::string> m_buildSceneList; // ビルドに含まれるシーンのリスト
    bool m_isBuilding = false;
    bool m_buildSucceeded = false;
    std::string m_buildResultMessage;
};
