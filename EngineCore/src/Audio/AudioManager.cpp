#include "AudioManager.h"


// AudioManagerの初期化
void AudioManager::init()
{
    // SoLoudコアを初期化
    SoLoud::result result = m_Soloud.init();

    // 初期化が成功したかを確認
    if (result != SoLoud::SO_NO_ERROR) {
        printf("AudioManager Initialize Field");
        return;
    }

	printf("AudioManager Initialize Success");
}

// AudioManagerの終了処理
void AudioManager::shutdown()
{
    // すべてのサウンドキャッシュをクリア
    m_SoundCache.clear();

    // SoLoudコアを終了させ、リソースを解放
    m_Soloud.deinit();

    printf("AudioManager Close");
}
void AudioManager::update3d_audio()
{
}

void AudioManager::play_sfx(const std::string& soundName)
{
	// サウンドをロードして再生
	SoLoud::Wav* sound = load_sound(soundName);
	if (sound) {
		m_Soloud.play(*sound);
	}
}

void AudioManager::play_sfx3d(const std::string& soundName)
{
	// サウンドをロードして再生
	SoLoud::Wav* sound = load_sound(soundName);
	if (sound) {
		// HACK: floatの配列で位置指定してる。Vector3とかでやりたい
		float pos[3] = { 5.0f, 0.0f, 0.0f };
		m_Soloud.play3d(*sound, pos[0], pos[1], pos[2]);
	}
}

SoLoud::Wav* AudioManager::load_sound(const std::string& soundName)
{
    // キャッシュに存在するかチェック
    if (m_SoundCache.count(soundName)) {
        return &m_SoundCache[soundName];
    }

    // 存在しなければロード
    std::string path = "assets/sounds/" + soundName;
    SoLoud::Wav& newSound = m_SoundCache[soundName];
    if (newSound.load(path.c_str()) != SoLoud::SO_NO_ERROR) {
        m_SoundCache.erase(soundName); // ロード失敗したら消す
        return nullptr;
    }
    return &newSound;
}
