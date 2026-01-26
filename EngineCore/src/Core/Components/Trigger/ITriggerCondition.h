#pragma once
#include "Core/Components/TriggerContext/TriggerContext.h"

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
};