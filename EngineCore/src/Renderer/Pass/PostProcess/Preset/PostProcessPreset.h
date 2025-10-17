#pragma once
#include <map>
#include <string>
#include <vector>

using PostProcessParameter = std::map<std::string, float>;
using PostProcessPassSettings = std::map<std::string, PostProcessParameter>;

struct PostProcessPreset
{
    std::string name;
    PostProcessPassSettings settings;
	std::vector<std::string> order;
};
