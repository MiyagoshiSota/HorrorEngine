#include "SceneRenderer.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Renderer/Graphics/DescriptorHeap/SrvDescriptorHeap.h"
#include "Scene/GameObject/IGameObjectBase.h"

SceneRenderer::SceneRenderer(std::wstring vsfilePath, std::wstring psfilePath)
{
	rootSignature = new RootSignature();
	if (!rootSignature->IsValid())
	{
		printf("ルートシグネチャの生成に失敗");
		return;
	}

	pipelineState = new PipelineState();
	pipelineState->SetInputLayout(SharedStruct::Vertex::InputLayout);
	pipelineState->SetRootSignature(rootSignature->Get());
	pipelineState->SetVS(L"../x64/Debug/SimpleVS.cso");
	pipelineState->SetPS(L"../x64/Debug/SimplePS.cso");

	pipelineState->SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
	pipelineState->SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT); // 深度フォーマットも明示的に指定


	pipelineState->Create();
	if (!pipelineState->IsValid())
	{
		printf("パイプラインステートの生成に失敗\n");
		return;
	}
}

void SceneRenderer::BeginFrame()
{
}

void SceneRenderer::UpdateFrame()
{
}

void SceneRenderer::EndFrame()
{
}

void SceneRenderer::DrawGameObject(ID3D12GraphicsCommandList* commandList, std::shared_ptr<IGameObjectBase> obj)
{
	//auto materialHeap = obj->GetModel()->m_Material->m_DescriptorHeap->GetHeap();
	for (size_t i = 0; i < obj->GetModel()->m_InputMesh.size(); i++)
	{
		auto vbView = obj->GetModel()->m_Meshes->m_VertexBuffer[i]->View();
		auto ibView = obj->GetModel()->m_Meshes->m_IndexBuffers[i]->View();

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vbView);
		commandList->IASetIndexBuffer(&ibView);

		//commandList->SetDescriptorHeaps(1, &materialHeap);
		commandList->SetGraphicsRootDescriptorTable(1, obj->GetModel()->m_Material->m_MaterialHandles[i]->gpuHandle);

		commandList->DrawIndexedInstanced(obj->GetModel()->m_InputMesh[i].Indeices.size(), 1, 0, 0, 0);
	}
}