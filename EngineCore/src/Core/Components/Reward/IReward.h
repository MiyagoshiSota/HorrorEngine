#pragma once
#include "Core/Components/TriggerContext/TriggerContext.h"

class GameObject;

class IReward
{
public:
    virtual ~IReward() = default;

    // アクションを実行する
    virtual void Execute(const TriggerContext& context) = 0;
    
    // GUIのインスペクターに、このアクション固有の設定項目を描画する
    virtual void DrawInspectorUI() = 0;

    // アクションの名前を取得する
    virtual std::string GetName() const = 0;
};