#pragma once
#include <memory>

#include "Light.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"

class LightingManager
{
public:
    void init();

    // DirectionalLightを追加する
    std::shared_ptr<Light> add_directional_light(LightType type,
                                                 const DirectX::XMFLOAT3& color,
                                                 float intensity,
                                                 const DirectX::XMFLOAT3& direction);;

    // PointLightを追加する
    std::shared_ptr<Light> add_point_light(LightType type,
                                           const DirectX::XMFLOAT3& color,
                                           float intensity,
                                           const DirectX::XMFLOAT3& position,
                                           float range,
                                           float attenuation);

    // SpotLightを追加する
    std::shared_ptr<Light> add_spot_light(LightType type,
                                          const DirectX::XMFLOAT3& color,
                                          float intensity,
                                          const DirectX::XMFLOAT3& position,
                                          const DirectX::XMFLOAT3& direction,
                                          float inngerAngle, float outerAngle,float range, float attenuation);

    // 毎フレーム呼び出し、定数バッファを更新する
    void update_constant_buffer() const;

    // 定数バッファを取得する
    std::shared_ptr<ConstantBuffer> get_constant_buffer() const { return m_LightingConstantBuffer; }

    std::vector<std::shared_ptr<Light>> get_lights()
    {
        auto lights = std::vector<std::shared_ptr<Light>>();

        // ディレクショナルライトを追加
        for (const auto& dirLight : m_DirectionalLights)
        {
            lights.push_back(dirLight);
        }

        // ポイントライトを追加
        for (const auto& pointLight : m_PointLights)
        {
            lights.push_back(pointLight);
        }

        for (const auto& spotLight : m_SpotLights)
        {
            lights.push_back(spotLight);
        }

        return lights;
    }

private:
    std::vector<std::shared_ptr<DirectionalLight>> m_DirectionalLights;
    std::vector<std::shared_ptr<PointLight>> m_PointLights;
    std::vector<std::shared_ptr<SpotLight>> m_SpotLights;
    std::shared_ptr<ConstantBuffer> m_LightingConstantBuffer;
};
