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
    DirectX::XMFLOAT2 BlurDirection; // (1,0)=horizontal, (0,1)=vertical (Separable時のみ使用)
    float StepSize;   // À-Trous のステップ (1, 2, 4, 8, 16)
    float Padding1;
};

/// デノイズモード: Off=コピーのみ, Bilateral, Separable, À-Trous
enum class RTAODenoiseMode
{
    Off,
    Bilateral,
    Separable,
    ATrous
};

class RTAODenoisePass : public IRenderPass
{
public:
    RTAODenoisePass();

    void Execute(RenderContext& context) override;

    void SetDenoiseMode(RTAODenoiseMode mode) { m_denoiseMode = mode; }
    RTAODenoiseMode GetDenoiseMode() const { return m_denoiseMode; }
    void SetDepthSigma(float v) { m_depthSigma = v; }
    void SetNormalSigma(float v) { m_normalSigma = v; }
    float GetDepthSigma() const { return m_depthSigma; }
    float GetNormalSigma() const { return m_normalSigma; }

private:
    std::shared_ptr<ConstantBuffer> m_constants;
    RTAODenoiseMode m_denoiseMode = RTAODenoiseMode::Bilateral;
    float m_depthSigma = 16.0f;
    float m_normalSigma = 8.0f;
};
