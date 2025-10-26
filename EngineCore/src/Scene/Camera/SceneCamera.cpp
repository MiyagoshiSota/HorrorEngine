#include "SceneCamera.h"

#include <algorithm>

#include "Core/App.h"
#include "Input/InputDevice.h"

void SceneCamera::Init()
{
    // Y-up座標系を前提とします（DirectXの標準）
    // もしZ-upが良い場合は、m_WorldUpを(0,0,1)にし、
    // SetEyePos/TargetPosのYとZを入れ替えてください。
    SetEyePos(0.0f, 100.0f, 300.0f, 1.0f); // X, Y, Z, W
    SetTargetPos(0.0f, 0.0f, 0.0f, 1.0f);
    m_WorldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    m_Fov = DirectX::XMConvertToRadians(90.0f);
    
    // アスペクト比はウィンドウサイズから計算
    m_Aspect = WINDOW_WIDTH / WINDOW_HEIGHT;

    // 感度設定
    m_MoveSpeed = 50.0f;
    m_RotationSpeed = 0.005f; // マウスの移動量1ピクセルあたりのラジアン
    m_ZoomSpeed = 0.01f;      // ホイール1目盛りあたりの処理量

    // 内部ベクトルを初期化
    UpdateCameraVectors();
}

void SceneCamera::UpdateCameraVectors()
{
    // Forward ベクトル (LookAt)
    m_Forward = DirectX::XMVector3Normalize(
        DirectX::XMVectorSubtract(m_TargetPos, m_EyePos)
    );

    // Right ベクトル
    m_Right = DirectX::XMVector3Normalize(
        DirectX::XMVector3Cross(m_WorldUp, m_Forward)
    );

    // Camera Up ベクトル (ForwardとRightに直交するベクトル)
    m_CameraUp = DirectX::XMVector3Cross(m_Forward, m_Right);
}

DirectX::XMMATRIX SceneCamera::GetViewMatrix() const
{
    return DirectX::XMMatrixLookAtLH(m_EyePos, m_TargetPos, m_CameraUp);
}

DirectX::XMMATRIX SceneCamera::GetProjectionMatrix() const
{
    return DirectX::XMMatrixPerspectiveFovLH(m_Fov, m_Aspect, 0.1f, 1000.0f); // 近クリップ/遠クリップ
}

void SceneCamera::Update(float deltaTime)
{
    // 入力デバイスの参照を取得
    auto& input = InputDevice::GetInstance();
    
    // マウスのデルタ（移動量）を取得
    const auto mouse_delta = input.GetMouseDelta();
    
    // ズーム (マウスホイール)
    // TODO:GetMouseWheelDeltaから正常な値が取れていない
    float scrollDelta = input.GetMouseWheelDelta();
    if (scrollDelta != 0.0f)
    {
        Zoom(scrollDelta);
    }

    // パン (中マウスボタン + ドラッグ)
    if (input.IsMouseDown(2)) // 2 = Middle Mouse Button
    {
        Pan(-mouse_delta.x * m_MoveSpeed * 0.01f, -mouse_delta.y * m_MoveSpeed * 0.01f);
    }
    // オービット (左マウスボタン + ドラッグ)
    else if (input.IsMouseDown(1)) // 0 = Left Mouse Button
    {
        Orbit(-mouse_delta.x * m_RotationSpeed * 0.5, m_RotationSpeed * mouse_delta.y * 0.5);
    }
    
    // FPS風移動 (WASD)
    float moveSpeed = m_MoveSpeed * deltaTime;
    DirectX::XMVECTOR moveDir = DirectX::XMVectorZero();

    if (input.IsKeyDown('W')) // Forward
    {
        moveDir = DirectX::XMVectorAdd(moveDir, m_Forward);
    }
    if (input.IsKeyDown('S')) // Back
    {
        moveDir = DirectX::XMVectorSubtract(moveDir, m_Forward);
    }
    if (input.IsKeyDown('A')) // Right
    {
        moveDir = DirectX::XMVectorAdd(moveDir, m_Right);
    }
    if (input.IsKeyDown('D')) // Left
    {
        moveDir = DirectX::XMVectorSubtract(moveDir, m_Right);
    }
    if (input.IsKeyDown(VK_SPACE)) // World Up
    {
        moveDir = DirectX::XMVectorAdd(moveDir, m_WorldUp);
    }
    if (input.IsKeyDown(VK_CONTROL)) // World Down
    {
        moveDir = DirectX::XMVectorSubtract(moveDir, m_WorldUp);
    }

    // 移動ベクトルを正規化してから速度を適用
    moveDir = DirectX::XMVector3Normalize(moveDir);
    moveDir = DirectX::XMVectorScale(moveDir, moveSpeed);

    // EyePosとTargetPosを同じだけ動かす（＝平行移動）
    m_EyePos = DirectX::XMVectorAdd(m_EyePos, moveDir);
    m_TargetPos = DirectX::XMVectorAdd(m_TargetPos, moveDir);

    // 変更を反映するためにベクトルを再計算
    UpdateCameraVectors();
}

void SceneCamera::Pan(float dx, float dy)
{
    // 右方向の移動ベクトル
    DirectX::XMVECTOR vRight = DirectX::XMVectorScale(m_Right, dx);
    // 上方向の移動ベクトル
    DirectX::XMVECTOR vUp = DirectX::XMVectorScale(m_CameraUp, dy);
    
    // EyeとTargetを同じだけ動かす
    m_EyePos = DirectX::XMVectorAdd(m_EyePos, vRight);
    m_EyePos = DirectX::XMVectorAdd(m_EyePos, vUp);
    m_TargetPos = DirectX::XMVectorAdd(m_TargetPos, vRight);
    m_TargetPos = DirectX::XMVectorAdd(m_TargetPos, vUp);
}

void SceneCamera::Orbit(float dYaw, float dPitch)
{
    // TargetからEyeへのベクトル
    DirectX::XMVECTOR vToEye = DirectX::XMVectorSubtract(m_EyePos, m_TargetPos);

    // 1. Yaw回転 (ワールドの上方向、Y軸周り)
    DirectX::XMMATRIX mRotYaw = DirectX::XMMatrixRotationAxis(m_WorldUp, dYaw);
    vToEye = DirectX::XMVector3Transform(vToEye, mRotYaw);
    m_CameraUp = DirectX::XMVector3Transform(m_CameraUp, mRotYaw); // カメラの上方向も一緒に回転

    // 2. Pitch回転 (カメラの右方向、m_Right軸周り)
    DirectX::XMMATRIX mRotPitch = DirectX::XMMatrixRotationAxis(m_Right, dPitch);
    vToEye = DirectX::XMVector3Transform(vToEye, mRotPitch);
    m_CameraUp = DirectX::XMVector3Transform(m_CameraUp, mRotPitch);

    // 新しいEyePosを計算
    m_EyePos = DirectX::XMVectorAdd(m_TargetPos, vToEye);

    // ジンバルロックを防ぐためにUpベクトルを再正規化
    m_Right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(m_WorldUp, m_Forward));
    m_CameraUp = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(m_Forward, m_Right));
}

void SceneCamera::Zoom(float dScroll)
{
    // m_TargetPos と m_EyePos の間の距離ベクトル
    DirectX::XMVECTOR vToTarget = DirectX::XMVectorSubtract(m_TargetPos, m_EyePos);

    // 距離を計算
    float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(vToTarget));
    
    // ズーム量を計算 (距離が近いほどゆっくりズームする)
    float zoomAmount = distance * m_ZoomSpeed * dScroll;

    // ズーム後の新しい距離
    float newDistance = distance - zoomAmount;

    // vToTarget を正規化（長さを1に）して、方向だけを取得します
    DirectX::XMVECTOR forwardDir = DirectX::XMVector3Normalize(vToTarget);
    
    // TargetPos から (逆方向 * newDistance) の位置に EyePos を設定します
    // TargetPos + (-forwardDir * newDistance) と同じ意味です
    m_EyePos = DirectX::XMVectorSubtract(m_TargetPos, DirectX::XMVectorScale(forwardDir, newDistance));
}