#pragma once

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include <DirectXMath.h>
#include <memory>

/// RTAOの空間的デノイズ（法線+距離のバイラテラル）
struct alignas(256) RTAODenoiseConstants
{
    DirectX::XMFLOAT3 CameraPosition;
    float DepthSigma;
    float NormalSigma;
    DirectX::XMFLOAT2 InvScreenSize;
    float Padding0;
};

class RTAODenoisePass : public IRenderPass
{
public:
    RTAODenoisePass();

    void Execute(RenderContext& context) override;

    void SetDepthSigma(float v) { m_depthSigma = v; }
    void SetNormalSigma(float v) { m_normalSigma = v; }
    float GetDepthSigma() const { return m_depthSigma; }
    float GetNormalSigma() const { return m_normalSigma; }

private:
    std::shared_ptr<ConstantBuffer> m_constants;
    float m_depthSigma = 16.0f;
    float m_normalSigma = 8.0f;
};
