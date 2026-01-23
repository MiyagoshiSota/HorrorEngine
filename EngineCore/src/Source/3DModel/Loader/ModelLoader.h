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
    bool Init();
    auto GetModel(const std::string& model_name) -> std::shared_ptr<Model>;
    bool SetModel(const std::string& model_name, const std::string& model_path);
    std::map<std::string, std::vector<SharedStruct::Mesh>> GetAllModelsData() const { return m_ModelDataCache; }
	std::map < std::string, std::shared_ptr<Model> > GetAllModels() const { return m_ModelCache; }

    std::vector<SharedStruct::Mesh> GetModelOriginData(const std::string& model_name);

private:
    bool Deserialize(const std::string& models_file_path);
    void CreatePrimitiveObjects();
    
private:
	// モデル名と、ロード済みのモデルデータのキャッシュ
    std::map<std::string, std::vector<SharedStruct::Mesh>> m_ModelDataCache;
    std::map < std::string, std::shared_ptr<Model> > m_ModelCache;
    
    // モデル名から実際のファイルパスなどを解決する辞書
    std::map<std::string, std::string> m_ModelRegistry; 
};