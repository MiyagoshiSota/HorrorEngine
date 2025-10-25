#pragma once
#include "imgui.h"
#include "ITriggerCondition.h"

class CountDownCondition : public ITriggerCondition
{
public:
    bool Check(const TriggerContext& context) override
    {
        // 経過時間を更新
        m_now_timer += context.m_DeltaTime;
        // タイマーが目標時間に達したかをチェック
        if (m_now_timer >= m_target_timer)
        {
            return true;
        }
        return false;
    }

    void DrawInspectorUI() override
    {
        // m_target_timerの設定UI
        ImGui::InputFloat("Target Timer (seconds)", &m_target_timer);
    }

    std::string GetName() const override
    {
        return "CountDownCondition";
    };

private:
    float m_now_timer = 0.0f;
    float m_target_timer = 5.0f; // デフォルトで5秒間
};
