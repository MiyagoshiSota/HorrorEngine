#pragma once
#include "Core/Components/TriggerContext/TriggerContext.h"
#include <nlohmann/json_fwd.hpp>

class GameObject;

class ITriggerCondition
{
public:
    virtual ~ITriggerCondition() = default;

    // 条件を判定する (毎フレーム、あるいは特定のイベント時に呼ばれる)
    virtual bool Check(const TriggerContext& context) = 0;

#ifndef BUILD_STANDALONE
    // GUIのインスペクターに、このトリガー固有の設定項目を描画する
    virtual void DrawInspectorUI() = 0;
#else
    // BUILD_STANDALONE時は空実装
    virtual void DrawInspectorUI() {}
#endif // BUILD_STANDALONE

    virtual std::string GetName() const = 0;

    /// シーン保存用。Condition 固有のパラメータを j に書き出す（name は呼び出し元で設定済み）
    virtual void Serialize(nlohmann::json& j) const {}
    /// シーン読込用。j から Condition 固有のパラメータを復元する
    virtual void Deserialize(const nlohmann::json& j) {}
};