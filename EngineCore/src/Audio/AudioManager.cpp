#include "AudioManager.h"


// AudioManagerの初期化
void AudioManager::Init()
{
    // SoLoudコアを初期化
    SoLoud::result result = m_Soloud.init();

    // 初期化が成功したかを確認
    if (result != SoLoud::SO_NO_ERROR) {
        printf("AudioManager Initialize Field\n");
        return;
    }

	printf("AudioManager Initialize Success\n");
}

// AudioManagerの終了処理
void AudioManager::Shutdown()
{
    // すべてのサウンドキャッシュをクリア
    m_SoundCache.clear();

    // SoLoudコアを終了させ、リソースを解放
    m_Soloud.deinit();

    printf("AudioManager Close\n");
}
void AudioManager::Update3dAudio()
{
}

void AudioManager::PlaySfx(const std::string& soundName)
{
	// サウンドをロードして再生
	SoLoud::Wav* sound = LoadSound(soundName);
	if (sound) {
		m_Soloud.play(*sound);
	}
}

void AudioManager::PlaySfx3d(const std::string& soundName)
{
	// サウンドをロードして再生
	SoLoud::Wav* sound = LoadSound(soundName);
	if (sound) {
		// HACK: floatの配列で位置指定してる。Vector3とかでやりたい
		float pos[3] = { 5.0f, 0.0f, 0.0f };
		m_Soloud.play3d(*sound, pos[0], pos[1], pos[2]);
	}
}

SoLoud::Wav* AudioManager::LoadSound(const std::string& soundName)
{
    // キャッシュに存在するかチェック
    if (m_SoundCache.count(soundName)) {
        return &m_SoundCache[soundName];
    }

    // 存在しなければロード
    std::string path = "Assets/sounds/" + soundName;
    SoLoud::Wav& newSound = m_SoundCache[soundName];
    if (newSound.load(path.c_str()) != SoLoud::SO_NO_ERROR) {
        m_SoundCache.erase(soundName); // ロード失敗したら消す
        return nullptr;
    }
    return &newSound;
}
