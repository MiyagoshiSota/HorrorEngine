#pragma once
#include "Core/Components/Trigger/ITriggerCondition.h"
#include "Core/Components/TriggerContext/TriggerContext.h"
#include "Scene/ScenePicker.h"

/// <summary>
/// このトリガーが付いた GameObject が、左クリックでピックされたときに true を返す Condition。
/// 対象オブジェクトには Rigidbody（コライダー）が必要。ScenePicker が毎フレーム更新されていること。
/// </summary>
class ClickOnObjectCondition : public ITriggerCondition
{
public:
    bool Check(const TriggerContext& context) override
    {
        if (context.m_Owner == nullptr)
            return false;
        return ScenePicker::GetInstance().DidClickThisFrame() &&
               ScenePicker::GetInstance().GetPickedObject() == context.m_Owner.get();
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        ImGui::TextUnformatted("Left-click on this object (with Rigidbody) to trigger.");
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "ClickOnObjectCondition";
    }
};
