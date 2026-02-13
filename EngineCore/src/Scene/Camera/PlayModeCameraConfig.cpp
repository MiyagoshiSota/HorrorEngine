#include "PlayModeCameraConfig.h"
#include <algorithm>

PlayModeCameraConfig& PlayModeCameraConfig::GetInstance()
{
	static PlayModeCameraConfig instance;
	return instance;
}

void PlayModeCameraConfig::AddFirstPersonRotation(float dYaw, float dPitch)
{
	m_firstPersonYaw += dYaw;
	m_firstPersonPitch += dPitch;
	const float pitchLimit = 1.4f; // 約80度
	m_firstPersonPitch = std::clamp(m_firstPersonPitch, -pitchLimit, pitchLimit);
}

void PlayModeCameraConfig::AddFollowOrbitRotation(float dYaw, float dPitch)
{
	m_followOrbitYaw += dYaw;
	m_followOrbitPitch += dPitch;
	const float pitchLimit = 1.4f; // 約80度（真上・真下を避ける）
	m_followOrbitPitch = std::clamp(m_followOrbitPitch, -pitchLimit, pitchLimit);
}
