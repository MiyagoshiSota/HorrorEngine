#pragma once
#include <mutex>

#include "Component.h"

class ComponentFactory
{
public:
	using CreateComponentFunc = std::unique_ptr<Component>(*)();

	static std::unique_ptr<Component> create(const std::string& type)
	{
		std::map<std::string, CreateComponentFunc>& mappings = get_mappings();
		auto it = mappings.find(type);
		if (it != mappings.end())
		{
			// 登録された関数を呼び出してComponentを生成
			return std::unique_ptr(it->second());
		}
		return nullptr;
	};

	template<typename T>
	static void Register(const std::string& type){
		get_mappings()[type] = []() -> std::unique_ptr<Component> { return std::make_unique<T>(); };
	}
	
	static std::map<std::string, CreateComponentFunc>& get_mappings()
	{
		static std::map<std::string, CreateComponentFunc> mappings;
		return mappings;
	};
};
