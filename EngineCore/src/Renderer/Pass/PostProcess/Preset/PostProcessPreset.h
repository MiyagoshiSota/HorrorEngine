#pragma once
#include <map>
#include <string>
#include <vector>
#include <optional> // ★ optionalヘッダーを追加

using PostProcessParameter = std::map<std::string, float>;
using PostProcessPassSettings = std::map<std::string, PostProcessParameter>;

struct PostProcessPreset
{
	std::string m_name;
	PostProcessPassSettings settings;
    
	// ★ 型をstd::optionalでラップ
	std::vector<std::string> order; 
};