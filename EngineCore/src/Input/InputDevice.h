#pragma once

#include <Windows.h>      // Windows API (MSG, WPARAM, LPARAM, VK_...)
#include <windowsx.h>   // GET_X_LPARAM, GET_Y_LPARAM
#include <DirectXMath.h>  // DirectX::XMFLOAT2

constexpr int MAX_KEYS = 256;
constexpr int MAX_MOUSE_BUTTONS = 3; // 0=Left, 1=Right, 2=Middle

class InputDevice
{
public:
    static InputDevice& GetInstance()
    {
        static InputDevice instance;
        return instance;
    }

    InputDevice(const InputDevice&) = delete;
    void operator=(const InputDevice&) = delete;

    /**
     * @brief 毎フレームの開始時に呼び出します。
     * キー/ボタンの「前フレーム」の状態を更新し、デルタ値をリセットします。
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void Update(float deltaTime);

    /**
     * @brief Windowsメッセージプロシージャ(WndProc)から呼び出します。
     * @param msg メッセージタイプ (e.g., WM_KEYDOWN)
     * @param wParam メッセージのWPARAM
     * @param lParam メッセージのLPARAM
     */
    void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // --- キーボード入力 ---

    /**
     * @brief キーが現在押下されているか（押しっぱなし）を返します。
     * @param vKey 仮想キーコード (例: 'W', VK_SPACE)
     */
    bool IsKeyDown(int vKey) const;

    /**
     * @brief キーがこのフレームで「新たに」押されたかを返します。
     * @param vKey 仮想キーコード
     */
    bool IsKeyPressed(int vKey) const;

    /**
     * @brief キーがこのフレームで離されたかを返します。
     * @param vKey 仮想キーコード
     */
    bool IsKeyReleased(int vKey) const;


    // --- マウス入力 ---

    /**
     * @brief マウスボタンが現在押下されているか（押しっぱなし）を返します。
     * @param button 0=Left, 1=Right, 2=Middle
     */
    bool IsMouseDown(int button) const;

    /**
     * @brief マウスボタンがこのフレームで「新たに」押されたかを返します。
     * @param button 0=Left, 1=Right, 2=Middle
     */
    bool IsMousePressed(int button) const;

    /**
     * @brief マウスボタンがこのフレームで離されたかを返します。
     * @param button 0=Left, 1=Right, 2=Middle
     */
    bool IsMouseReleased(int button) const;

    /**
     * @brief 現在のマウスカーソル位置（クライアント座標）を返します。
     */
    DirectX::XMFLOAT2 GetMousePosition() const;

    /**
     * @brief 前のフレームからのマウスの移動量（デルタ）を返します。
     */
    DirectX::XMFLOAT2 GetMouseDelta() const;

    /**
     * @brief このフレームでのマウスホイールのスクロール量（デルタ）を返します。
     */
    float GetMouseWheelDelta();

    /**
     * @brief キーが押され続けている時間（秒）を返します。
     * @param vKey 仮想キーコード
     * @return キーが押され続けている時間（秒）。押されていない場合は0.0f
     */
    float GetKeyDownDuration(int vKey) const;

private:
    // プライベートコンストラクタ（シングルトン）
    InputDevice();
    ~InputDevice() = default;

    // キーボード状態
    bool m_keyState[MAX_KEYS];
    bool m_prevKeyState[MAX_KEYS];

    // マウスボタン状態
    bool m_mouseState[MAX_MOUSE_BUTTONS];
    bool m_prevMouseState[MAX_MOUSE_BUTTONS];

    // マウス位置
    int m_mouseX;
    int m_mouseY;
    int m_prevMouseX;
    int m_prevMouseY;

    // マウスデルタ
    int m_mouseDeltaX;
    int m_mouseDeltaY;
    float m_mouseWheelDelta;

    // キーが押され続けている時間（秒）
    float m_keyDownDuration[MAX_KEYS];
};