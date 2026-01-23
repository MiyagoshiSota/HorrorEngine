#include "InputDevice.h"
#include <cstring> // for memset

InputDevice::InputDevice()
    : m_mouseX(0), m_mouseY(0),
      m_prevMouseX(0), m_prevMouseY(0),
      m_mouseDeltaX(0), m_mouseDeltaY(0),
      m_mouseWheelDelta(0.0f)
{
    // すべてのキー状態を false (離されている) で初期化
    memset(m_keyState, false, sizeof(m_keyState));
    memset(m_prevKeyState, false, sizeof(m_prevKeyState));
    memset(m_mouseState, false, sizeof(m_mouseState));
    memset(m_prevMouseState, false, sizeof(m_prevMouseState));
    // キーが押され続けている時間を0で初期化
    memset(m_keyDownDuration, 0, sizeof(m_keyDownDuration));
}

void InputDevice::Update(float deltaTime)
{
    // 現在の状態を前のフレームの状態としてコピー
    memcpy(m_prevKeyState, m_keyState, sizeof(m_keyState));
    memcpy(m_prevMouseState, m_mouseState, sizeof(m_mouseState));

    // キーが押され続けている時間を更新
    for (int i = 0; i < kMaxKeys; i++)
    {
        if (m_keyState[i])
        {
            m_keyDownDuration[i] += deltaTime;
        }
        else
        {
            m_keyDownDuration[i] = 0.0f;
        }
    }

    // マウスのデルタを計算
    m_mouseDeltaX = m_mouseX - m_prevMouseX;
    m_mouseDeltaY = m_mouseY - m_prevMouseY;

    // マウスの現在位置を「前回の位置」として保存
    m_prevMouseX = m_mouseX;
    m_prevMouseY = m_mouseY;
}

void InputDevice::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // キーボード
    case WM_KEYDOWN:
        m_keyState[wParam] = true;
        break;
    case WM_KEYUP:
        m_keyState[wParam] = false;
        break;

    // マウスボタン
    case WM_LBUTTONDOWN:
        m_mouseState[0] = true;
        break;
    case WM_LBUTTONUP:
        m_mouseState[0] = false;
        break;
    case WM_RBUTTONDOWN:
        m_mouseState[1] = true;
        break;
    case WM_RBUTTONUP:
        m_mouseState[1] = false;
        break;
    case WM_MBUTTONDOWN:
        m_mouseState[2] = true;
        break;
    case WM_MBUTTONUP:
        m_mouseState[2] = false;
        break;

    // マウス移動
    case WM_MOUSEMOVE:
        m_mouseX = GET_X_LPARAM(lParam);
        m_mouseY = GET_Y_LPARAM(lParam);
        break;

    // マウスホイール
    case WM_MOUSEWHEEL:
        m_mouseWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA);
        break;
    }
}

// --- キーボード ゲッター ---
bool InputDevice::IsKeyDown(int vKey) const
{
    return m_keyState[vKey];
}

bool InputDevice::IsKeyPressed(int vKey) const
{
    // 現在押されていて、前は押されていなかった
    return m_keyState[vKey] && !m_prevKeyState[vKey];
}

bool InputDevice::IsKeyReleased(int vKey) const
{
    // 現在押されていなくて、前は押されていた
    return !m_keyState[vKey] && m_prevKeyState[vKey];
}


// --- マウス ゲッター ---
bool InputDevice::IsMouseDown(int button) const
{
    if (button < 0 || button >= kMaxMouseButtons) return false;
    return m_mouseState[button];
}

bool InputDevice::IsMousePressed(int button) const
{
    if (button < 0 || button >= kMaxMouseButtons) return false;
    // 現在押されていて、前は押されていなかった
    return m_mouseState[button] && !m_prevMouseState[button];
}

bool InputDevice::IsMouseReleased(int button) const
{
    if (button < 0 || button >= kMaxMouseButtons) return false;
    // 現在押されていなくて、前は押されていた
    return !m_mouseState[button] && m_prevMouseState[button];
}

DirectX::XMFLOAT2 InputDevice::GetMousePosition() const
{
    return {static_cast<float>(m_mouseX), static_cast<float>(m_mouseY)};
}

DirectX::XMFLOAT2 InputDevice::GetMouseDelta() const
{
    return {static_cast<float>(m_mouseDeltaX), static_cast<float>(m_mouseDeltaY)};
}

float InputDevice::GetMouseWheelDelta()
{
    float delta = m_mouseWheelDelta;
    m_mouseWheelDelta = 0.0f;
    return delta;
}

float InputDevice::GetKeyDownDuration(int vKey) const
{
    if (vKey < 0 || vKey >= kMaxKeys) return 0.0f;
    return m_keyDownDuration[vKey];
}
