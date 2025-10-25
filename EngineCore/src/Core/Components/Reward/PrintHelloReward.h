#pragma once
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"

class PrintHelloReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        printf("hello\n");
        // サウンドを再生するロジックを実装
        // 例: AudioManagerを使ってサウンドを再生
        // if (context.audioManager)
        // {
        //     context.audioManager->playSound(owner->getSoundID());
        // }
    }

    void DrawInspectorUI() override
    {
        // インスペクターに表示する設定項目があればここに実装
        // 例えば、再生するサウンドのIDやボリュームなどを設定できるようにする
    }

    std::string GetName() const override
    {
        return "PrintHelloAction";
    }
};
