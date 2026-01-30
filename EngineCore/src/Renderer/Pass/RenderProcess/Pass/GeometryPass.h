#pragma once
#include "Renderer/Pass/RenderProcess/SceneRenderPassBase.h"
#include "Renderer/RenderContext/RenderContext.h"
#include <DirectXMath.h>

class GeometryPass : public SceneRenderPassBase
{
public:
    GeometryPass()
        : m_prevViewProj(DirectX::XMMatrixIdentity())
        , m_isFirstFrame(true)
    {
    }

protected:
    void Collect(RenderContext& context) override;
    void Draw(RenderContext& context) override;

private:
    DirectX::XMMATRIX m_prevViewProj; // 前フレームのViewProj行列（Motion Vector用）
    bool m_isFirstFrame; // 最初のフレームかどうか
};