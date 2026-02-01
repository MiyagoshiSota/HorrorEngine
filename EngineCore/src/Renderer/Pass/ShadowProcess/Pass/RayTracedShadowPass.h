#pragma once
#include "Renderer/Pass/IRenderPass.h"

/// Ray Traced Hard Shadow Pass
/// DXRを使用してハードシャドウを計算する（リソースはRayTracedShadowManagerが所有）
class RayTracedShadowPass : public IRenderPass
{
public:
    RayTracedShadowPass() = default;
    ~RayTracedShadowPass() override = default;

    void Execute(RenderContext& context) override;

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

private:
    bool m_enabled = false;
};
