#include "Core/App.h"
#include <DirectXTex.h>

#include "Scene/Default/Scene/DefaultScene.h"
#include "Modules/PublicConst/ConstNamePref.h"
#include "Modules/PublicConst/ConstBuildPref.h"
#include "Modules/PublicConst/ConstPathPref.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#ifdef BUILD_STANDALONE
// スタンドアロンビルド時はbuild_config.jsonからシーンパスを読み込む
std::string LoadScenePathFromConfig()
{
	std::filesystem::path configPath = ConstBuildPref::kBuildConfigJsonPath;
	
	if (!std::filesystem::exists(configPath))
	{
		printf("Warning: build_config.json not found. Using default scene.\n");
		return ConstPathPref::kDefaultGameObjectPath;
	}

	std::ifstream file(configPath);
	if (!file.is_open())
	{
		printf("Warning: Failed to open build_config.json. Using default scene.\n");
		return ConstPathPref::kDefaultGameObjectPath;
	}

		try
		{
			nlohmann::json configJson;
			file >> configJson;
			
			// シーンリストをチェック
			if (configJson.contains(ConstBuildPref::kSceneListKey) && configJson[ConstBuildPref::kSceneListKey].is_array())
			{
				auto sceneList = configJson[ConstBuildPref::kSceneListKey];
				if (!sceneList.empty())
				{
					std::string scenePath = sceneList[0].get<std::string>();
					printf("Loading scene from build_config.json: %s\n", scenePath.c_str());
					
					// プロジェクトルートからの相対パス（Game/Assets/...）を実行時の相対パス（Assets/...）に変換
					// build_config.jsonにはプロジェクトルートからの相対パスが保存されているが、
					// 実行ファイルはBuilds/Release/から実行されるため、Assets/から始まるパスに変換する必要がある
					if (scenePath.find("Game/Assets/") == 0)
					{
						scenePath = scenePath.substr(5); // "Game/"を削除して"Assets/..."にする
						printf("Converted scene path: %s\n", scenePath.c_str());
					}
					else if (scenePath.find("Game\\Assets\\") == 0)
					{
						scenePath = scenePath.substr(5); // "Game\"を削除して"Assets\..."にする
						std::replace(scenePath.begin(), scenePath.end(), '\\', '/'); // パス区切りを統一
						printf("Converted scene path: %s\n", scenePath.c_str());
					}
					
					return scenePath;
				}
			}
		
		printf("Warning: scene_list not found in build_config.json. Using default scene.\n");
		return ConstPathPref::kDefaultGameObjectPath;
	}
	catch (const std::exception& e)
	{
		printf("Warning: Failed to parse build_config.json: %s. Using default scene.\n", e.what());
		return ConstPathPref::kDefaultGameObjectPath;
	}
}
#endif

int main() {
	std::shared_ptr<ISceneBase> scene = std::make_shared<DefaultScene>();
	
#ifdef BUILD_STANDALONE
	// スタンドアロンビルド時はPlayModeで起動
	ChangeSceneType(SceneType::PlayMode);
	std::string scenePath = LoadScenePathFromConfig();
	StartApp(TEXT(ConstNamePref::WindowName), scene, scenePath);
#else
	StartApp(TEXT(ConstNamePref::WindowName), scene);
#endif
	
	ShutdownApp();
	return 0;
}