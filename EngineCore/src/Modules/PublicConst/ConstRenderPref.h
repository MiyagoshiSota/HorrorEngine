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
	static inline const char* GBufferAlbedo = "GBufferAlbedo";
	static inline const char* GBufferMaterial = "GBufferMaterial";  // roughness, metallic, AO, emissive.r
	static inline const char* GBufferEmissive = "GBufferEmissive";   // emissive.g, emissive.b
	static inline const char* SSAOBuffer = "SSAOBuffer";
	static inline const char* RtShadowVisibility = "RtShadowVisibility";
};
