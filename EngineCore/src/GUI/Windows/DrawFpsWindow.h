#pragma once

#include "GUI/Core/IDrawWindow.h"

class DrawFpsWindow : public IDrawWindow
{
public:
	DrawFpsWindow() = default;
	~DrawFpsWindow() override = default;

	void draw() override;
};
