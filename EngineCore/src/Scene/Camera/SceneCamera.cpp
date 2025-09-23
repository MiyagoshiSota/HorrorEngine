#include <d3dx12.h>
#include "SceneCamera.h"
#include "Renderer/Engine.h"
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Core/App.h"
#include <DirectXMath.h>

void SceneCamera::SetInformation()
{
	eyePos = DirectX::XMVectorSet(0.0f, 120.0, 300.0, 0.0f);
	targetPos = DirectX::XMVectorSet(0.0f, 120.0, 0.0, 0.0f);
	upward = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	fov = DirectX::XMConvertToRadians(60);
	aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT); // アスペクト比
}

DirectX::XMVECTOR SceneCamera::GetEyePos()
{
	return eyePos;
}

DirectX::XMVECTOR SceneCamera::GetTargetPos()
{
	return targetPos;
}

DirectX::XMVECTOR SceneCamera::GetUpward()
{
	return upward;
}

float SceneCamera::GetFOV()
{
	return fov;
}

float SceneCamera::GetAspect()
{
	return aspect;
}
