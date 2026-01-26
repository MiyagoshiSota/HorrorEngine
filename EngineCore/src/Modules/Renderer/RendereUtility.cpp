#include "RendereUtility.h"

#include <d3dx12.h>
#include "Renderer/RenderContext/RenderContext.h"

void RendererUtility::simple_change_target_state(
	std::shared_ptr<std::vector<D3D12_RESOURCE_BARRIER>> barriers, 
	std::shared_ptr<ITargetBase> target,
	D3D12_RESOURCE_STATES change_state)
{
	if (target->GetCurrentState() != change_state)
	{
		barriers->push_back(CD3DX12_RESOURCE_BARRIER::Transition(
			target->GetResource(),
			target->GetCurrentState(),
			change_state
		));
		// Targetクラスの状態も更新
		//target->SetCurrentState(change_state);
	}
}

void RendererUtility::ResolveMSAA(RenderContext& context, const std::string& msaaSourceName, const std::string& resolveDestName)
{
	auto cmdList = context.CommandList;
	auto msaaColorRT = context.GetRenderTarget(msaaSourceName);
	auto sceneColorRT = context.GetRenderTarget(resolveDestName);

	if (!msaaColorRT || !sceneColorRT)
	{
		return;
	}

	// Barrier: SceneColor -> ResolveDest
	if (sceneColorRT->GetCurrentState() == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			sceneColorRT->GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		cmdList->ResourceBarrier(1, &barrier);
	}

	D3D12_RESOURCE_BARRIER resolveBarriers[2];
	resolveBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
		msaaColorRT->GetResource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE
	);
	resolveBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
		sceneColorRT->GetResource(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_RESOLVE_DEST
	);
	cmdList->ResourceBarrier(2, resolveBarriers);

	cmdList->ResolveSubresource(
		sceneColorRT->GetResource(), 0,
		msaaColorRT->GetResource(), 0,
		sceneColorRT->GetResource()->GetDesc().Format
	);

	// Barrier Restore
	std::vector<D3D12_RESOURCE_BARRIER> barriersPost;
	barriersPost.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
		msaaColorRT->GetResource(),
		D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	));
	barriersPost.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
		sceneColorRT->GetResource(),
		D3D12_RESOURCE_STATE_RESOLVE_DEST,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	));
	cmdList->ResourceBarrier(static_cast<UINT>(barriersPost.size()), barriersPost.data());

	sceneColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
	msaaColorRT->SetCurrentState(D3D12_RESOURCE_STATE_RENDER_TARGET);
}
