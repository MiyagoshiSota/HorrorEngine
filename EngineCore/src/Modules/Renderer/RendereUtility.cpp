#include "RendereUtility.h"

#include <d3dx12.h>

void RendererUtility::simple_change_target_state(
	std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers, 
	std::shared_ptr<ITargetBase> target,
	D3D12_RESOURCE_STATES change_state)
{
	if (target->GetCurrentState() != change_state)
	{
		barriers->push_back(CD3DX12_RESOURCE_BARRIER::Transition(
			target->GetResource().Get(),
			target->GetCurrentState(),
			change_state
		));
		// Targetクラスの状態も更新
		//target->SetCurrentState(change_state);
	}
}
