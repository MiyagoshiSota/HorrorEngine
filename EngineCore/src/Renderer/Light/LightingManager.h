#pragma once
#include <memory>

#include "Light.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"

class LightingManager
{
public:
    void init();

    // シーンにライトを追加する
    std::shared_ptr<Light> add_light(LightType type);
    
    // 毎フレーム呼び出し、定数バッファを更新する
    void update_constant_buffer() const;
    
    // 定数バッファを取得する
    std::shared_ptr<ConstantBuffer> get_constant_buffer() const { return m_LightingConstantBuffer; }

private:
    std::vector<std::shared_ptr<Light>> m_Lights;
    std::shared_ptr<ConstantBuffer> m_LightingConstantBuffer;
};
