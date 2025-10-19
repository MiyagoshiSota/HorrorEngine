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

    // 毎フレーム呼び出し、定数バッファを更新する
    void update_constant_buffer() const;

    // 定数バッファを取得する
    std::shared_ptr<ConstantBuffer> get_constant_buffer() const { return m_LightingConstantBuffer; }

private:
    std::vector<std::shared_ptr<DirectionalLight>> m_DirectionalLights;
    std::vector<std::shared_ptr<PointLight>> m_PointLights;
    std::shared_ptr<ConstantBuffer> m_LightingConstantBuffer;
};
