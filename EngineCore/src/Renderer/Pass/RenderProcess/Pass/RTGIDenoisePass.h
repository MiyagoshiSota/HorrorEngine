#pragma once

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/Pass/RenderProcess/Pass/RTAODenoisePass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include <DirectXMath.h>
#include <memory>

/// RTGI用デノイズパス（RTAODenoiseと同一アルゴリズム、RGB入力出力）
class RTGIDenoisePass : public IRenderPass
{
public:
    RTGIDenoisePass();

    void Execute(RenderContext& context) override;

    void SetDenoiseMode(RTAODenoiseMode mode) { m_denoiseMode = mode; }
    RTAODenoiseMode GetDenoiseMode() const { return m_denoiseMode; }
    void SetDepthSigma(float v) { m_depthSigma = v; }
    void SetNormalSigma(float v) { m_normalSigma = v; }
    float GetDepthSigma() const { return m_depthSigma; }
    float GetNormalSigma() const { return m_normalSigma; }

private:
    std::shared_ptr<ConstantBuffer> m_constants;
    RTAODenoiseMode m_denoiseMode = RTAODenoiseMode::Off;
    float m_depthSigma = 16.0f;
    float m_normalSigma = 8.0f;
};
