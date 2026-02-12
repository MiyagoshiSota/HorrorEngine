#pragma once

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>

/// SSR合成用定数バッファ（b0）
struct alignas(256) SSRCompositeConstants
{
    float ReflectionIntensity;
    float MaxRoughness;  // この値以上は反射を弱める
    float Padding[2];
};

/// SceneColor + SSRBuffer を roughness に応じてブレンドし、指定 RTV に出力
class SSRCompositePass : public IRenderPass
{
public:
    SSRCompositePass();

    void Execute(RenderContext& context) override;

    void SetReflectionIntensity(float intensity) { m_reflectionIntensity = intensity; }
    void SetMaxRoughness(float r) { m_maxRoughness = r; }

private:
    std::shared_ptr<ConstantBuffer> m_compositeConstants;
    float m_reflectionIntensity = 1.0f;
    float m_maxRoughness = 0.6f;
};
