#pragma once

#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/RenderContext/ShadowTypes.h"
#include "Modules/PublicConst/ConstRenderPref.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>

/// ライティングパス用定数バッファ（b0）
struct alignas(256) LightingTransformCB
{
    DirectX::XMFLOAT3 CameraPosition;
    float Padding0;
    DirectX::XMFLOAT4X4 LightViewProj;
    int ShadowMode;
    DirectX::XMFLOAT2 InvRayTracedShadowMapSize; // ShadowMode==RayTracedMask 時: スクリーンUV用
    int Padding1[1];
};

/// G-Buffer + シャドウから最終カラーを計算するフルスクリーンパス
class LightingPass : public IRenderPass
{
public:
    LightingPass();

    void Execute(RenderContext& context) override;

private:
    std::shared_ptr<ConstantBuffer> m_lightingTransformCB;
};
