#pragma once

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>

/// SSAO用定数バッファ（b0）
struct alignas(256) SSAOConstants
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 InvProjection;
    DirectX::XMFLOAT4X4 Projection;
    DirectX::XMFLOAT4 ProjectionParams; // x=far, y=1/far, z=screenWidth, w=screenHeight
    float Radius;
    float Bias;
    float Power;
    float Padding0;
};

/// 深度・法線バッファからスクリーン空間アンビエントオクルージョンを計算し、AOマップを出力する
class SSAOPass : public IRenderPass
{
public:
    SSAOPass();

    void Execute(RenderContext& context) override;

    void SetRadius(float radius) { m_radius = radius; }
    void SetBias(float bias) { m_bias = bias; }
    void SetPower(float power) { m_power = power; }
    float GetRadius() const { return m_radius; }
    float GetBias() const { return m_bias; }
    float GetPower() const { return m_power; }

private:
    std::shared_ptr<ConstantBuffer> m_ssaoConstants;
    float m_radius = 0.5f;
    float m_bias = 0.025f;
    float m_power = 2.0f;
};
