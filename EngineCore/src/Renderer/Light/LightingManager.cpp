#include "LightingManager.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h" 

void LightingManager::init()
{
    m_LightingConstantBuffer = std::make_unique<ConstantBuffer>(sizeof(SharedStruct::LightingParams));
}

std::shared_ptr<Light> LightingManager::add_light(LightType type)
{
    auto newLight = std::make_shared<Light>();
	newLight->Intensity = 10.0f;
	newLight->Color = { 1.0f, 0.1f, 0.1f };
	newLight->Direction = { -1.0f, -1.0f, -1.0f };
    m_Lights.push_back(newLight);
    newLight->Type = type;
    return newLight;
}

void LightingManager::update_constant_buffer() const
{
    auto* params = m_LightingConstantBuffer->GetPtr<SharedStruct::LightingParams>();
    
    params->AmbientColor = { 1.0f, 0.2f, 0.2f, 0.2f }; // 環境光を仮設定
    params->NumLights = static_cast<int>(m_Lights.size());

    // シーンの全ライトの情報をシェーダー用の構造体にコピー
    for (int i = 0; i < m_Lights.size(); ++i)
    {
        const auto& light = m_Lights[i];
        auto& data = params->Lights[i];

        data.Position = { light->Position.x, light->Position.y, light->Position.z, (float)light->Type };
        data.Color = { light->Color.x, light->Color.y, light->Color.z, light->Intensity };
    }   
}
