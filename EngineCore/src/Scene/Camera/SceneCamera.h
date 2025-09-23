#pragma once
#include <DirectXMath.h>

class SceneCamera
{
public:
	void Init();

	DirectX::XMVECTOR GetEyePos() const { return m_EyePos; }
	DirectX::XMVECTOR GetTargetPos() const { return m_TargetPos; }
	DirectX::XMVECTOR GetUpward() const { return m_Upward; }
	float GetFOV() const { return m_Fov; }
	float GetAspect() const { return m_Aspect; }

	void SetEyePos(float pos1,double pos2,double pos3,float pos4) { 
		m_EyePos = DirectX::XMVectorSet(pos1, pos2, pos3, pos4); 
	}
	void SetTargetPos(float pos1, double pos2, double pos3, float pos4) {
		m_TargetPos = DirectX::XMVectorSet(pos1, pos2, pos3, pos4);
	}
	void SetFOV(float fov) { m_Fov = fov; }
	void SetAspect(float aspect) { m_Aspect = aspect; }


private:
	DirectX::XMVECTOR m_EyePos;
	DirectX::XMVECTOR m_TargetPos;
	DirectX::XMVECTOR m_Upward;
	float m_Fov;
	float m_Aspect;
};

