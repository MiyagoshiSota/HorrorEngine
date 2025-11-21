#pragma once
#include <map>
#include <string>
#include <memory>
#include <vector>

#include "Renderer/StandardShader/Struct/SharedStruct.h"

class Model; // 前方宣言

class ModelLoader
{
public:
    bool init();
    auto GetModel(const std::string& model_name) -> std::shared_ptr<Model>;
    bool set_model(const std::string& model_name, const std::string& model_path);
    std::map<std::string, std::vector<SharedStruct::Mesh>> get_all_models_data() const { return m_ModelDataCache; }
	std::map < std::string, std::shared_ptr<Model> > get_all_models() const { return m_ModelCache; }

    std::vector<SharedStruct::Mesh> GetModelOriginData(const std::string& model_name);

private:
    bool desirialize(const std::string& models_file_path);
    void create_primitive_objects();
    
private:
	// モデル名と、ロード済みのモデルデータのキャッシュ
    std::map<std::string, std::vector<SharedStruct::Mesh>> m_ModelDataCache;
    std::map < std::string, std::shared_ptr<Model> > m_ModelCache;
    
    // モデル名から実際のファイルパスなどを解決する辞書
    std::map<std::string, std::string> m_ModelRegistry; 
};