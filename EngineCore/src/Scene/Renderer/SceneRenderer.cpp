#include "SceneRenderer.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "Renderer/Graphics/DescriptorHeap.h"

SceneRenderer::SceneRenderer(std::wstring vsfilePath, std::wstring psfilePath)
{
	rootSignature = new RootSignature();
	if (!rootSignature->IsValid())
	{
		printf("ルートシグネチャの生成に失敗");
		return ;
	}

	pipelineState = new PipelineState();
	pipelineState->SetInputLayout(SharedStruct::Vertex::InputLayout);
	pipelineState->SetRootSignature(rootSignature->Get());
	pipelineState->SetVS(L"../x64/Debug/SimpleVS.cso");
	pipelineState->SetPS(L"../x64/Debug/SimplePS.cso");
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

// 引数から currentIndex と constantBuffer を削除
void SceneRenderer::DrawGameObject(ID3D12GraphicsCommandList* commandList, std::shared_ptr<GameObjectBase> obj)
{
	auto materialHeap = obj->m_Model->m_Material->m_DescriptorHeap->GetHeap();
	for (size_t i = 0; i < obj->m_Model->m_InputMesh.size(); i++)
	{
		auto vbView = obj->m_Model->m_Meshes->m_VertexBuffer[i]->View();
		auto ibView = obj->m_Model->m_Meshes->m_IndexBuffers[i]->View();

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vbView);
		commandList->IASetIndexBuffer(&ibView);

		commandList->SetDescriptorHeaps(1, &materialHeap);
		commandList->SetGraphicsRootDescriptorTable(1, obj->m_Model->m_Material->m_MaterialHandles[i]->HandleGPU);

		commandList->DrawIndexedInstanced(obj->m_Model->m_InputMesh[i].Indeices.size(), 1, 0, 0, 0);
	}
}