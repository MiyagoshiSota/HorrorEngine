#pragma once
#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE
#include "ITriggerCondition.h"
#include <nlohmann/json.hpp>

class CountDownCondition : public ITriggerCondition
{
public:
    bool Check(const TriggerContext& context) override
    {
        // 経過時間を更新
        m_nowTimer += context.m_DeltaTime;
        // タイマーが目標時間に達したかをチェック
        if (m_nowTimer >= m_targetTimer)
        {
            return true;
        }
        return false;
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        // m_targetTimerの設定UI
        ImGui::InputFloat("Target Timer (seconds)", &m_targetTimer);
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "CountDownCondition";
    };

    void Serialize(nlohmann::json& j) const override
    {
        j["targetTimer"] = m_targetTimer;
    }
    void Deserialize(const nlohmann::json& j) override
    {
        if (j.contains("targetTimer"))
            m_targetTimer = j["targetTimer"].get<float>();
    }

private:
    float m_nowTimer = 0.0f;
    float m_targetTimer = 5.0f; // デフォルトで5秒間
};
