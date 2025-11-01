#include "LightingManager.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h" 

void LightingManager::init()
{
    m_LightingConstantBuffer = std::make_shared<ConstantBuffer>(sizeof(SharedStruct::LightingParams));
}

std::shared_ptr<Light> LightingManager::add_directional_light(LightType type, const DirectX::XMFLOAT3& color,
    float intensity, const DirectX::XMFLOAT3& direction)
{
    auto newLight = std::make_shared<DirectionalLight>();
    newLight->Color = color;
    newLight->Intensity = intensity;
    newLight->Direction = direction;
    newLight->Type = type;
    m_DirectionalLights.push_back(newLight);
	return newLight;
}

std::shared_ptr<Light> LightingManager::add_point_light(LightType type, const DirectX::XMFLOAT3& color, float intensity,
                                                        const DirectX::XMFLOAT3& position, float range, float attenuation)
{
    auto newLight = std::make_shared<PointLight>();
    newLight->Color = color;
    newLight->Intensity = intensity;
    newLight->Position = position;
    newLight->Type = type;
    newLight->Range = range;
    newLight->Attenuation = attenuation;
    m_PointLights.push_back(newLight);
	return newLight;
}

void LightingManager::update_constant_buffer() const
{
	SharedStruct::LightingParams lightingParams = {};

	// 環境光の設定
	lightingParams.AmbientColor = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

	// 平行光源の設定
	lightingParams.NumDirectionalLights = static_cast<int>(m_DirectionalLights.size());
	for (size_t i = 0; i < m_DirectionalLights.size() && i < 4; ++i)
	{
		const auto& light = m_DirectionalLights[i];
		lightingParams.DirectionalLights[i].Direction = DirectX::XMFLOAT4(
			light->Direction.x,
			light->Direction.y,
			light->Direction.z,
			0.0f);
		lightingParams.DirectionalLights[i].ColorAndIntensity = DirectX::XMFLOAT4(
			light->Color.x * light->Intensity,
			light->Color.y * light->Intensity,
			light->Color.z * light->Intensity,
			1.0f);
	}

	// 点光源の設定
	lightingParams.NumPointLights = static_cast<int>(m_PointLights.size());
	for (size_t i = 0; i < m_PointLights.size() && i < 28; ++i)
	{
		const auto& light = m_PointLights[i];
		lightingParams.PointLights[i].Position = DirectX::XMFLOAT4(
			light->Position.x,
			light->Position.y,
			light->Position.z,
			1.0f);
		lightingParams.PointLights[i].ColorAndIntensity = DirectX::XMFLOAT4(
			light->Color.x * light->Intensity,
			light->Color.y * light->Intensity,
			light->Color.z * light->Intensity,
			1.0f);
		lightingParams.PointLights[i].AttenuationAndRange = DirectX::XMFLOAT4(
			light->Attenuation,
			light->Range,
			0.0f,
			0.0f);
	}

	// 定数バッファの書き込み先ポインタを取得
	auto* pGpuData = m_LightingConstantBuffer->GetPtr<SharedStruct::LightingParams>();
    
	// ローカルで準備したデータを、GPUが見るメモリ領域に丸ごとコピー
	memcpy(pGpuData, &lightingParams, sizeof(SharedStruct::LightingParams));
}
