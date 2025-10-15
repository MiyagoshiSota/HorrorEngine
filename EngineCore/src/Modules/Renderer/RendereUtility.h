#pragma once
#include <memory>
#include <vector>

#include "Renderer/Target/ITargetBase.h"

class RendererUtility
{
public:
	/// <summary>
	/// ターゲットの状態を簡易的に変更する。
	/// </summary>
	/// <param name="barriers"></param>
	/// <param name="target"></param>
	/// <param name="now_states"></param>
	/// <param name="change_state"></param>
	static void	simple_change_target_state(std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers, std::shared_ptr<ITargetBase> target, D3D12_RESOURCE_STATES change_state);
};

