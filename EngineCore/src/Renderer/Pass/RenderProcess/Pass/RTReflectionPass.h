#pragma once
#include "Renderer/Pass/IRenderPass.h"

/// Ray Traced Reflection Pass
/// G-Buffer（ワールド位置・法線・ラフネス）から反射方向を計算し DXR で TraceRay、結果を LightingPass で合成する
class RTReflectionPass : public IRenderPass
{
public:
    RTReflectionPass() = default;
    ~RTReflectionPass() override = default;

    void Execute(RenderContext& context) override;

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetBias(float bias) { m_bias = bias; }
    void SetMaxDistance(float d) { m_maxDistance = d; }
    void SetReflectionIntensity(float intensity) { m_reflectionIntensity = intensity; }
    void SetRoughnessThreshold(float t) { m_roughnessThreshold = t; }
    void SetFresnelF0(float F0) { m_fresnelF0 = F0; }
    float GetBias() const { return m_bias; }
    float GetMaxDistance() const { return m_maxDistance; }
    float GetReflectionIntensity() const { return m_reflectionIntensity; }
    float GetRoughnessThreshold() const { return m_roughnessThreshold; }
    float GetFresnelF0() const { return m_fresnelF0; }

private:
    bool m_enabled = false;
    float m_bias = 0.01f;
    float m_maxDistance = 100.0f;
    float m_reflectionIntensity = 1.0f;
    float m_roughnessThreshold = 0.9f;
    float m_fresnelF0 = 0.04f;
};
