#pragma once

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>

/// SSR用定数バッファ（b0）
struct alignas(256) SSRConstants
{
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 InvView;
    DirectX::XMFLOAT4X4 Projection;
    DirectX::XMFLOAT4X4 InvProjection;
    DirectX::XMFLOAT4 ProjectionParams; // x=farZ, y=1/farZ, z=screenWidth, w=screenHeight
    float NearZ;
    float FarZ;
    float MaxRayDistance;
    float RayStep;
    float MaxSteps;
    float Thickness;
    float Enable;
    float Padding[2];
};

/// スクリーンスペースリフレクション：SceneColor + Depth + G-Buffer から反射カラーをレイマーチで求め SSRBuffer に出力
class SSRPass : public IRenderPass
{
public:
    SSRPass();

    void Execute(RenderContext& context) override;

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    void SetMaxRayDistance(float d) { m_maxRayDistance = d; }
    void SetRayStep(float s) { m_rayStep = s; }
    void SetMaxSteps(int s) { m_maxSteps = s; }
    void SetThickness(float t) { m_thickness = t; }

private:
    std::shared_ptr<ConstantBuffer> m_ssrConstants;
    bool m_enabled = true;
    float m_maxRayDistance = 50.0f;
    float m_rayStep = 1.0f;
    int m_maxSteps = 64;
    float m_thickness = 0.1f;
};
