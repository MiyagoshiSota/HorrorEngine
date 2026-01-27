#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Renderer/Target/ITargetBase.h"

class RenderContext;

class RendererUtility
{
public:
	/// <summary>
	/// ターゲットの状態を簡易的に変更する。
	/// </summary>
	/// <param name="barriers"></param>
	/// <param name="target"></param>
	/// <param name="change_state"></param>
	static void	simple_change_target_state(std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers, std::shared_ptr<ITargetBase> target, D3D12_RESOURCE_STATES change_state);

	/// <summary>
	/// MSAAレンダーターゲットを非MSAAレンダーターゲットにリゾルブする
	/// </summary>
	/// <param name="context">レンダーコンテキスト</param>
	/// <param name="msaaSourceName">MSAAソースレンダーターゲットの名前</param>
	/// <param name="resolveDestName">リゾルブ先レンダーターゲットの名前</param>
	static void ResolveMSAA(RenderContext& context, const std::string& msaaSourceName, const std::string& resolveDestName);
};

