#pragma once
#include <map>
#include <string>

using PostProcessParameter = std::map<std::string, float>;
using PostProcessPassSettings = std::map<std::string, PostProcessParameter>;

struct PostProcessPreset
{
    std::string name;
    PostProcessPassSettings settings;
};
