#pragma once
#include <DirectXMath.h>

class SceneCamera
{
public:
	void SetInformation();
	DirectX::XMVECTOR GetEyePos();
	DirectX::XMVECTOR GetTargetPos();
	DirectX::XMVECTOR GetUpward();
	float GetFOV();
	float GetAspect();

private:
	DirectX::XMVECTOR eyePos;
	DirectX::XMVECTOR targetPos;
	DirectX::XMVECTOR upward;
	float fov;
	float aspect;
};

