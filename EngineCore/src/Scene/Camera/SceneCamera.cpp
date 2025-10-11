#include "SceneCamera.h"
#include "Core/App.h"

void SceneCamera::Init()
{
	SetEyePos(0.0f, 120.0, 100.0, 0.0f);
	SetTargetPos(0.0f, 50.0, 0.0, 0.0f);
	SetFOV(30);

	m_Upward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	auto aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
	SetAspect(aspect);
}
