#pragma once
#include "soloud.h"
#include "soloud_wav.h"
#include <map>
#include <string>

class AudioManager
{
public:
    void init();
    void shutdown();
    static void update3d_audio();

    /// <summary>
	/// 2Dサウンド再生
    /// </summary>
    /// <param name="soundName"></param>
    void play_sfx(const std::string& soundName);

    /// <summary>
	/// 3Dサウンド再生
    /// </summary>
    /// <param name="soundName"></param>
    void play_sfx3d(const std::string& soundName);

private:
    SoLoud::Soloud m_Soloud;

    // サウンド名をキーとして、ロード済みのサウンドリソースを保持するマップ
    std::map<std::string, SoLoud::Wav> m_SoundCache;

    // サウンドをロードまたはキャッシュから取得するヘルパー関数
    SoLoud::Wav* load_sound(const std::string& soundName);
};