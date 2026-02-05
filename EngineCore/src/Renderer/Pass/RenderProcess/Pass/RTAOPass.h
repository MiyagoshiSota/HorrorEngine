#pragma once
#include "Renderer/Pass/IRenderPass.h"

/// Ray Traced Ambient Occlusion Pass
/// G-Buffer（ワールド位置・法線）とTLASからレイトレでAOを計算し、SSAOBufferと同様にLightingPassで使用可能なテクスチャを出力する
class RTAOPass : public IRenderPass
{
public:
    RTAOPass() = default;
    ~RTAOPass() override = default;

    void Execute(RenderContext& context) override;

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetRadius(float radius) { m_radius = radius; }
    void SetBias(float bias) { m_bias = bias; }
    void SetNumRaysPerPixel(UINT n) { m_numRaysPerPixel = n; }
    float GetRadius() const { return m_radius; }
    float GetBias() const { return m_bias; }
    UINT GetNumRaysPerPixel() const { return m_numRaysPerPixel; }

private:
    bool m_enabled = false;
    float m_radius = 0.5f;
    float m_bias = 0.01f;
    UINT m_numRaysPerPixel = 1;
};
