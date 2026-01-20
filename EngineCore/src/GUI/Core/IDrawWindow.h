#pragma once

class IDrawWindow
{
public:
    virtual ~IDrawWindow() = default;
    virtual void draw() = 0;
    
    // ウィンドウの表示/非表示の管理
    virtual bool is_visible() const { return m_isVisible; }
    virtual void set_visible(bool visible) { m_isVisible = visible; }

protected:
    bool m_isVisible = true;  // デフォルトで表示
};
