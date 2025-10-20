#pragma once

class IDrawWindow
{
public:
    virtual ~IDrawWindow() = default;
    virtual void draw() = 0;
};
