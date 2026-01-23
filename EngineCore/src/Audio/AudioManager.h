#pragma once
#include "soloud.h"
#include "soloud_wav.h"
#include <map>
#include <string>

class AudioManager
{
public:
    void Init();
    void Shutdown();
    static void Update3dAudio();

    /// <summary>
	/// 2Dサウンド再生
    /// </summary>
    /// <param name="soundName"></param>
    void PlaySfx(const std::string& soundName);

    /// <summary>
	/// 3Dサウンド再生
    /// </summary>
    /// <param name="soundName"></param>
    void PlaySfx3d(const std::string& soundName);

private:
    SoLoud::Soloud m_Soloud;

    // サウンド名をキーとして、ロード済みのサウンドリソースを保持するマップ
    std::map<std::string, SoLoud::Wav> m_SoundCache;

    // サウンドをロードまたはキャッシュから取得するヘルパー関数
    SoLoud::Wav* LoadSound(const std::string& soundName);
};