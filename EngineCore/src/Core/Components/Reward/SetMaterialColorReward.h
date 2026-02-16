#pragma once
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/GameObject/Find/GameObjectFInder.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/MeshRenderer.h"
#include "Scene/GameObject/Model/Model.h"
#include <DirectXMath.h>
#include <string>

/// 名前で指定した GameObject の MeshRenderer に対し、指定マテリアルインデックスの色をインスタンス用にオーバーライドする Reward。
/// 同じモデルを使う他オブジェクトには影響しない。
class SetMaterialColorReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        auto target = GameObjectFinder::FindGameObjectsByName(m_targetName);
        if (target == nullptr)
            return;

        auto meshRenderer = target->FindComponent<MeshRenderer>();
        if (meshRenderer == nullptr || meshRenderer->model == nullptr)
            return;

        if (m_materialIndex >= meshRenderer->model->m_Materials.size())
            return;

        meshRenderer->SetMaterialColorOverride(m_materialIndex, m_color);
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        constexpr size_t kNameSize = 256;
        char nameBuf[kNameSize];
        strncpy_s(nameBuf, m_targetName.c_str(), kNameSize - 1);
        nameBuf[kNameSize - 1] = '\0';
        if (ImGui::InputText("Target Name", nameBuf, kNameSize))
            m_targetName = nameBuf;

        int materialIndexI = static_cast<int>(m_materialIndex);
        if (ImGui::DragInt("Material Index", &materialIndexI, 1.0f, 0, 1024))
            m_materialIndex = materialIndexI >= 0 ? static_cast<size_t>(materialIndexI) : 0u;

        ImGui::ColorEdit4("Color", &m_color.x);
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "SetMaterialColorReward";
    }

private:
    std::string m_targetName;
    size_t m_materialIndex = 0u;
    DirectX::XMFLOAT4 m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
};
