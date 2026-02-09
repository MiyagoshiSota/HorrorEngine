#pragma once
#include "Renderer/Pass/IRenderPass.h"

/// Ray Traced Global Illumination Pass（1-bounce間接光）
/// G-Buffer（ワールド位置・法線）とTLASからレイトレで間接光を計算し、LightingPassで加算する
class RTGIPass : public IRenderPass
{
public:
    RTGIPass() = default;
    ~RTGIPass() override = default;

    void Execute(RenderContext& context) override;

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetRadius(float radius) { m_radius = radius; }
    void SetBias(float bias) { m_bias = bias; }
    void SetIndirectIntensity(float intensity) { m_indirectIntensity = intensity; }
    void SetNumRaysPerPixel(UINT n) { m_numRaysPerPixel = n; }
    float GetRadius() const { return m_radius; }
    float GetBias() const { return m_bias; }
    float GetIndirectIntensity() const { return m_indirectIntensity; }
    UINT GetNumRaysPerPixel() const { return m_numRaysPerPixel; }

private:
    bool m_enabled = false;
    float m_radius = 2.0f;
    float m_bias = 0.01f;
    float m_indirectIntensity = 1.0f;
    UINT m_numRaysPerPixel = 1;
};
