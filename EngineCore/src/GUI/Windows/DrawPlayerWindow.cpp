#include "DrawPlayerWindow.h"

#ifdef BUILD_STANDALONE
void DrawPlayerWindow::draw() {}
#else

#include "Core/App.h"
#include "Scene/Character/Player/Player.h"
#include "Scene/Camera/PlayModeCameraConfig.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/ComponentFactory.h"
#include "Modules/PublicConst/ConstPrimitiveGameObjectPref.h"
#include "imgui.h"
#include <cstring>
#include <nlohmann/json.hpp>

void DrawPlayerWindow::draw()
{
	if (!ImGui::Begin("Player Window", &m_isVisible))
	{
		ImGui::End();
		return;
	}

	DrawPlayerGameObjectSection();
	ImGui::Separator();
	DrawPrimitiveAsPlayerSection();
	ImGui::Separator();
	DrawPlayModeCameraSection();
	ImGui::Separator();
	DrawInventorySection();

	ImGui::End();
}

void DrawPlayerWindow::DrawPlayerGameObjectSection()
{
	ImGui::Text("Player GameObject");
	ImGui::Separator();

	auto& player = Player::GetInstance();

	if (player.HasPlayerGameObject())
	{
		const auto go = player.GetPlayerGameObject();
		ImGui::Text("Current: %s", go ? go->GetName().c_str() : "(null)");

		if (ImGui::Button("Clear Player"))
		{
			player.SetPlayerGameObject(nullptr);
		}
	}
	else
	{
		ImGui::Text("Current: (none)");
	}

	ImGui::Spacing();
	ImGui::Text("Spawn");
	if (ImGui::Button("Spawn Player"))
	{
		if (!player.Spawn())
			ImGui::OpenPopup("SpawnFailed");
	}
	if (ImGui::BeginPopup("SpawnFailed"))
	{
		ImGui::Text("Player already exists.");
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	ImGui::Spacing();
	ImGui::Text("Set from Scene");
	if (!g_Scene)
	{
		ImGui::TextDisabled("No scene.");
		return;
	}

	const auto& gameObjects = g_Scene->GetGameObjects();
	if (gameObjects.empty())
	{
		ImGui::TextDisabled("No objects in scene.");
		return;
	}

	if (m_selectedSceneObjectIndex < 0 || m_selectedSceneObjectIndex >= static_cast<int>(gameObjects.size()))
		m_selectedSceneObjectIndex = 0;

	const std::string& currentName = gameObjects[m_selectedSceneObjectIndex]->GetName();
	if (ImGui::BeginCombo("Scene Object", currentName.c_str()))
	{
		for (int i = 0; i < static_cast<int>(gameObjects.size()); ++i)
		{
			const bool selected = (m_selectedSceneObjectIndex == i);
			if (ImGui::Selectable(gameObjects[i]->GetName().c_str(), selected))
				m_selectedSceneObjectIndex = i;
			if (selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (ImGui::Button("Set as Player"))
	{
		player.SetPlayerGameObject(gameObjects[m_selectedSceneObjectIndex]);
	}
}

void DrawPlayerWindow::DrawPrimitiveAsPlayerSection()
{
	ImGui::Text("Create Primitive as Player");
	ImGui::Separator();

	if (!g_Scene || !g_ModelLoader)
	{
		ImGui::TextDisabled("Scene or ModelLoader not available.");
		return;
	}

	auto& player = Player::GetInstance();

	auto createPrimitiveAndSetPlayer = [&player](const char* modelName, const char* displayName) -> bool
	{
		auto newGameObject = std::make_shared<GameObject>();
		newGameObject->m_name = displayName;

		auto rendererComponent = ComponentFactory::create("MeshRenderer");
		if (!rendererComponent)
			return false;
		rendererComponent->Initialize(newGameObject);
		rendererComponent->Deserialize(nlohmann::json{ {"model_name", modelName} });
		newGameObject->components.push_back(std::move(rendererComponent));

		newGameObject->Init();
		g_Scene->AddGameObject(newGameObject);
		player.SetPlayerGameObject(newGameObject);
		return true;
	};

	if (ImGui::Button("Cube as Player"))
	{
		createPrimitiveAndSetPlayer(ConstPrimitiveGameObjectPref::kCubeGameObjectKey, "PlayerCube");
	}
	ImGui::SameLine();
	if (ImGui::Button("Quad as Player"))
	{
		createPrimitiveAndSetPlayer(ConstPrimitiveGameObjectPref::kQuadGameObjectKey, "PlayerQuad");
	}
}

void DrawPlayerWindow::DrawPlayModeCameraSection()
{
	ImGui::Text("Play Mode Camera");
	ImGui::Separator();

	auto& config = PlayModeCameraConfig::GetInstance();

	const char* modeNames[] = { "Free", "First Person", "Follow" };
	int currentMode = static_cast<int>(config.GetMode());
	if (ImGui::Combo("Mode", &currentMode, modeNames, 3))
		config.SetMode(static_cast<PlayModeCameraConfig::Mode>(currentMode));

	if (config.GetMode() == PlayModeCameraConfig::Mode::FirstPerson)
	{
		DirectX::XMFLOAT3 offset = config.GetFirstPersonEyeOffset();
		if (ImGui::DragFloat3("Eye Offset", &offset.x, 0.1f))
			config.SetFirstPersonEyeOffset(offset.x, offset.y, offset.z);
	}
	else if (config.GetMode() == PlayModeCameraConfig::Mode::Follow)
	{
		float dist = config.GetFollowDistance();
		if (ImGui::DragFloat("Follow Distance", &dist, 0.1f, 0.1f, 50.0f))
			config.SetFollowDistance(dist);
		float height = config.GetFollowHeight();
		if (ImGui::DragFloat("Follow Height", &height, 0.1f, -10.0f, 20.0f))
			config.SetFollowHeight(height);
		float lookAtH = config.GetFollowLookAtHeight();
		if (ImGui::DragFloat("Look At Height", &lookAtH, 0.1f, -10.0f, 20.0f))
			config.SetFollowLookAtHeight(lookAtH);
		float smooth = config.GetFollowSmoothSpeed();
		if (ImGui::DragFloat("Smooth Speed", &smooth, 0.5f, 0.0f, 30.0f))
			config.SetFollowSmoothSpeed(smooth);
	}
}

void DrawPlayerWindow::DrawInventorySection()
{
	ImGui::Text("Inventory");
	ImGui::Separator();

	auto& player = Player::GetInstance();
	const auto& inventory = player.GetInventory();

	for (size_t i = 0; i < inventory.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		ImGui::Text("%s", inventory[i].c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Equip"))
		{
			player.EquipItem(inventory[i]);
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove"))
		{
			player.RemoveItem(inventory[i]);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	ImGui::Spacing();
	ImGui::Text("Held Item");
	const auto held = player.GetHeldItemInstance();
	if (held)
	{
		ImGui::Text("%s", held->GetName().c_str());
		ImGui::SameLine();
		if (ImGui::SmallButton("Unequip"))
			player.UnequipItem();
	}
	else
		ImGui::TextDisabled("(none)");

	if (inventory.empty())
		ImGui::TextDisabled("(empty)");

	ImGui::Spacing();
	ImGui::Text("Add Item");
	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("##ItemId", m_newItemIdBuffer, kItemIdBufferSize);
	ImGui::SameLine();
	if (ImGui::Button("Add"))
	{
		if (m_newItemIdBuffer[0] != '\0')
		{
			player.AddItem(m_newItemIdBuffer);
			m_newItemIdBuffer[0] = '\0';
		}
	}

	ImGui::Spacing();
	if (!inventory.empty() && ImGui::Button("Clear All"))
		player.ClearInventory();
}
#endif // BUILD_STANDALONE
