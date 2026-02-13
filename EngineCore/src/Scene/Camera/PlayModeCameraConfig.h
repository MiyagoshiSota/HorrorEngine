#pragma once

#include <DirectXMath.h>

/// <summary>
/// PlayMode 時のカメラ挙動を決める専用設定。シングルトンで保持し、PlayerWindow から編集・DefaultScene::Update で適用する。
/// </summary>
class PlayModeCameraConfig
{
public:
	enum class Mode
	{
		Free,        // 入力でカメラのみ移動（従来の Editor 風）
		FirstPerson, // カメラ＝プレイヤー＋目のオフセット、注視＝前方
		Follow       // カメラ＝プレイヤー後方オフセット、スムージング可
	};

	static PlayModeCameraConfig& GetInstance();

	PlayModeCameraConfig(const PlayModeCameraConfig&) = delete;
	PlayModeCameraConfig& operator=(const PlayModeCameraConfig&) = delete;

	Mode GetMode() const { return m_mode; }
	void SetMode(Mode mode) { m_mode = mode; }

	// --- FirstPerson ---
	DirectX::XMFLOAT3 GetFirstPersonEyeOffset() const { return m_firstPersonEyeOffset; }
	void SetFirstPersonEyeOffset(float x, float y, float z) { m_firstPersonEyeOffset = { x, y, z }; }
	float GetFirstPersonYaw() const { return m_firstPersonYaw; }
	void SetFirstPersonYaw(float v) { m_firstPersonYaw = v; }
	float GetFirstPersonPitch() const { return m_firstPersonPitch; }
	void SetFirstPersonPitch(float v) { m_firstPersonPitch = v; }
	void AddFirstPersonRotation(float dYaw, float dPitch);

	// --- Follow ---
	float GetFollowOrbitYaw() const { return m_followOrbitYaw; }
	void SetFollowOrbitYaw(float v) { m_followOrbitYaw = v; }
	float GetFollowOrbitPitch() const { return m_followOrbitPitch; }
	void SetFollowOrbitPitch(float v) { m_followOrbitPitch = v; }
	void AddFollowOrbitRotation(float dYaw, float dPitch);

	float GetFollowDistance() const { return m_followDistance; }
	void SetFollowDistance(float v) { m_followDistance = v; }
	float GetFollowHeight() const { return m_followHeight; }
	void SetFollowHeight(float v) { m_followHeight = v; }
	float GetFollowLookAtHeight() const { return m_followLookAtHeight; }
	void SetFollowLookAtHeight(float v) { m_followLookAtHeight = v; }
	float GetFollowSmoothSpeed() const { return m_followSmoothSpeed; }
	void SetFollowSmoothSpeed(float v) { m_followSmoothSpeed = v; }
	float GetRotationSensitivity() const { return m_rotationSensitivity; }
	void SetRotationSensitivity(float v) { m_rotationSensitivity = v; }

private:
	PlayModeCameraConfig() = default;
	~PlayModeCameraConfig() = default;

	Mode m_mode = Mode::Free;

	DirectX::XMFLOAT3 m_firstPersonEyeOffset = { 0.0f, 1.6f, 0.0f };

	float m_followDistance = 5.0f;
	float m_followHeight = 2.0f;
	float m_followLookAtHeight = 1.0f;
	float m_followSmoothSpeed = 8.0f;
	float m_followOrbitYaw = 0.0f;
	float m_followOrbitPitch = 0.0f;

	float m_firstPersonYaw = 0.0f;
	float m_firstPersonPitch = 0.0f;
	float m_rotationSensitivity = 0.0025f;
};
