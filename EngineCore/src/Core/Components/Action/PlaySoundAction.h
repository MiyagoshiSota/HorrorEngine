#pragma once
#include "IAction.h"
#include "Core/Components/TriggerContext/TriggerContext.h"

class PlaySoundAction : public IAction
{
public:
    void Execute(const TriggerContext& context) override
    {
        printf("hello");
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
};
