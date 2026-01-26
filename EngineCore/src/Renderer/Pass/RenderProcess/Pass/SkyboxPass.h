#pragma once
#include "Renderer/Pass/IRenderPass.h"
#include "Renderer/RenderContext/RenderContext.h"
#include <memory>

/// <summary>
/// キューブマップを使用したSkybox描画パス
/// </summary>
class SkyboxPass : public IRenderPass
{
public:
    SkyboxPass() = default;
    ~SkyboxPass() = default;

    /// <summary>
    /// レンダーパスの実行
    /// </summary>
    void Execute(RenderContext& context) override;

    /// <summary>
    /// Skyboxが有効かどうか（RenderContextから判定）
    /// </summary>
    bool IsEnabled(const RenderContext& context) const;

    /// <summary>
    /// Skyboxの有効/無効を設定
    /// </summary>
    void SetEnabled(bool enabled) { m_enabled = enabled; }

private:
    bool m_enabled = true;
};
