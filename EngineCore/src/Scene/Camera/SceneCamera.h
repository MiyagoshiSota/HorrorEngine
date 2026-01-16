#pragma once
#include <DirectXMath.h>

class InputDevice; 

class SceneCamera
{
public:
    void Init();

    DirectX::XMVECTOR GetEyePos() const { return m_EyePos; }
    DirectX::XMFLOAT3 GetEyePosFloat3() const {
        DirectX::XMFLOAT3 eyePos;
        DirectX::XMStoreFloat3(&eyePos, m_EyePos);
        return eyePos;
	}
    DirectX::XMVECTOR GetTargetPos() const { return m_TargetPos; }
    DirectX::XMVECTOR GetUpward() const { return m_CameraUp; } // m_Upwardから変更
    float GetFOV() const { return m_Fov; }
    float GetAspect() const { return m_Aspect; }

    void SetEyePos(float x, float y, float z, float w = 0.0f) { 
        m_EyePos = DirectX::XMVectorSet(x, y, z, w); 
    }
    void SetTargetPos(float x, float y, float z, float w = 0.0f) {
        m_TargetPos = DirectX::XMVectorSet(x, y, z, w);
    }
    void SetFOV(float fov) { m_Fov = fov; }
    void SetAspect(float aspect) { m_Aspect = aspect; }

    /**
     * @brief 毎フレーム呼び出し、入力に基づいてカメラを更新します。
     * @param input 入力デバイス（マウス/キーボードの状態）
     * @param deltaTime 前フレームからの経過時間
     */
    void Update(float deltaTime);

    /**
     * @brief ビュー行列を計算して返します。
     */
    DirectX::XMMATRIX GetViewMatrix() const;

    /**
     * @brief 射影行列を計算して返します。
     */
    DirectX::XMMATRIX GetProjectionMatrix() const;


private:
    /**
     * @brief m_EyePos と m_TargetPos に基づいて、
     * m_Forward, m_Right, m_CameraUp ベクトルを再計算します。
     */
    void UpdateCameraVectors();

    /**
     * @brief カメラをパン（上下左右に平行移動）させます。
     * @param dx スクリーンX軸方向の移動量
     * @param dy スクリーンY軸方向の移動量
     */
    void Pan(float dx, float dy);

    /**
     * @brief m_TargetPos を中心にカメラを回転（オービット）させます。
     * @param dYaw 水平方向の回転量（ラジアン）
     * @param dPitch 垂直方向の回転量（ラジアン）
     */
    void Orbit(float dYaw, float dPitch);

    /**
     * @brief m_TargetPos に向かってカメラをズームイン/アウトさせます。
     * @param dScroll マウスホイールの移動量
     */
    void Zoom(float dScroll);

private:
    // --- カメラの基本パラメータ ---
    DirectX::XMVECTOR m_EyePos;    // カメラの位置
    DirectX::XMVECTOR m_TargetPos; // 注視点
    float m_Fov;                   // 視野角 (ラジアン)
    float m_Aspect;                // アスペクト比

    // --- カメラのローカル座標系ベクトル ---
    DirectX::XMVECTOR m_Forward;   // Z軸 (見ている方向)
    DirectX::XMVECTOR m_Right;     // X軸 (右方向)
    DirectX::XMVECTOR m_CameraUp;  // Y軸 (上方向)

    // --- 固定値 ---
    DirectX::XMVECTOR m_WorldUp;   // ワールド座標系の上 (例: (0, 1, 0) または (0, 0, 1))

    // --- 操作感度 ---
    float m_MoveSpeed = 50.0f;     // パンの速度
    float m_RotationSpeed = 0.005f; // 回転の感度
    float m_ZoomSpeed = 0.01f;     // ズームの感度
};