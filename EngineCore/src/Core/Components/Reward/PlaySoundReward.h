#pragma once
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"

class PlaySoundReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        printf("Demo\n");
        // サウンドを再生するロジックを実装
        // 例: AudioManagerを使ってサウンドを再生
        // if (context.audioManager)
        // {
        //     context.audioManager->playSound(owner->getSoundID());
        // }
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        // インスペクターに表示する設定項目があればここに実装
        // 例えば、再生するサウンドのIDやボリュームなどを設定できるようにする
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "PlaySoundAction";
    }
};
