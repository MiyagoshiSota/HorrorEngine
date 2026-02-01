#pragma once

class ConstRenderPref
{
public:
	// RenderTargetName
	static inline const char* SceneColor = "SceneColor";
	static inline const char* SceneDepth = "SceneDepth";
	static inline const char* TmpColorA = "TmpColorA";
	static inline const char* TmpColorB = "TmpColorB";
	static inline const char* MSAART = "MSAART";
	static inline const char* MSAA_Depth = "MSAADepth";
	static inline const char* ShadowMap = "ShadowMap";
	static inline const char* CascadedShadowMap = "CascadedShadowMap";
	static inline const char* HistoryBuffer = "HistoryBuffer";
	static inline const char* MotionVectorBuffer = "MotionVectorBuffer";
	static inline const char* NormalBuffer = "NormalBuffer";
	static inline const char* WorldPositionBuffer = "WorldPositionBuffer";
};
