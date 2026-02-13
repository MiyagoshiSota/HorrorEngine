#pragma once

#include "GUI/Core/IDrawWindow.h"
#include <string>
#include <memory>

class GameObject;

class DrawPlayerWindow : public IDrawWindow
{
public:
	DrawPlayerWindow() = default;
	~DrawPlayerWindow() override = default;

	void draw() override;

private:
	void DrawPlayerGameObjectSection();
	void DrawPrimitiveAsPlayerSection();
	void DrawPlayModeCameraSection();
	void DrawInventorySection();

	static constexpr int kItemIdBufferSize = 128;
	char m_newItemIdBuffer[kItemIdBufferSize] = {};
	int m_selectedSceneObjectIndex = -1;
};
