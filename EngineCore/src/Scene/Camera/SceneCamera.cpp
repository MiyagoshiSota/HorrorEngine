#include "SceneCamera.h"

#include <algorithm>

#include "Core/App.h"
#include "Input/InputDevice.h"
#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif

void SceneCamera::Init()
{
    // Y-up座標系を前提とします（DirectXの標準）
    // もしZ-upが良い場合は、m_WorldUpを(0,0,1)にし、
    // SetEyePos/TargetPosのYとZを入れ替えてください。
    SetEyePos(0.0f, 1.0f, 3.0f, 1.0f); // X, Y, Z, W
    SetTargetPos(0.0f, 0.0f, 0.0f, 1.0f);
    m_WorldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    m_Fov = DirectX::XMConvertToRadians(90.0f);
    
    // アスペクト比はウィンドウサイズから計算
    m_Aspect = static_cast<float>(kWindowWidth) / static_cast<float>(kWindowHeight);

    // 感度設定
    m_MoveSpeed = 10.0f;
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
    // Reversed-Z: NearZ/FarZ を入れ替えると depth が 1 at near, 0 at far になる（DirectX 公式）。
    return DirectX::XMMatrixPerspectiveFovLH(m_Fov, m_Aspect, m_FarPlane, m_NearPlane);
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
    // FPS風回転 (右マウスボタン + ドラッグ)
    else if (input.IsMouseDown(1)) // 1 = Right Mouse Button
    {
        Rotate(-mouse_delta.x * m_RotationSpeed * 0.5f, m_RotationSpeed * mouse_delta.y * 0.5f);
    }
    
    // FPS風移動 (WASD) — ImGui入力中は無視
#ifndef BUILD_STANDALONE
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;
#endif
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
    if (input.IsKeyDown('A')) // Left
    {
        moveDir = DirectX::XMVectorSubtract(moveDir, m_Right);
    }
    if (input.IsKeyDown('D')) // Right
    {
        moveDir = DirectX::XMVectorAdd(moveDir, m_Right);
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
    // オービット: m_TargetPos を中心にカメラが周回する

    // TargetからEyeへのベクトル
    DirectX::XMVECTOR vToEye = DirectX::XMVectorSubtract(m_EyePos, m_TargetPos);

    // Yaw回転
    DirectX::XMMATRIX mRotYaw = DirectX::XMMatrixRotationAxis(m_WorldUp, dYaw);
    vToEye = DirectX::XMVector3Transform(vToEye, mRotYaw);

    // Pitch回転
    DirectX::XMMATRIX mRotPitch = DirectX::XMMatrixRotationAxis(m_Right, dPitch);
    vToEye = DirectX::XMVector3Transform(vToEye, mRotPitch);

    // 新しいEyePosを計算
    m_EyePos = DirectX::XMVectorAdd(m_TargetPos, vToEye);

    // ベクトルを再計算
    UpdateCameraVectors();
}

void SceneCamera::Rotate(float dYaw, float dPitch)
{
    // EyeからTargetへの距離を保存
    DirectX::XMVECTOR vToTarget = DirectX::XMVectorSubtract(m_TargetPos, m_EyePos);
    float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(vToTarget));

    // Yaw回転
    DirectX::XMMATRIX mRotYaw = DirectX::XMMatrixRotationAxis(m_WorldUp, -dYaw);
    m_Forward = DirectX::XMVector3Normalize(DirectX::XMVector3Transform(m_Forward, mRotYaw));

    // Pitch回転
    m_Right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(m_WorldUp, m_Forward));
    
    DirectX::XMMATRIX mRotPitch = DirectX::XMMatrixRotationAxis(m_Right, dPitch);
    DirectX::XMVECTOR newForward = DirectX::XMVector3Normalize(DirectX::XMVector3Transform(m_Forward, mRotPitch));

    // Pitch角度の制限（真上・真下を向かないように）
    float dotUp = DirectX::XMVectorGetY(newForward);
    const float pitchLimit = 0.95f; // 約72度
    if (dotUp > -pitchLimit && dotUp < pitchLimit)
    {
        m_Forward = newForward;
    }

    // 新しいTargetPosを計算（EyePosからForward方向にdistance分進んだ位置）
    m_TargetPos = DirectX::XMVectorAdd(m_EyePos, DirectX::XMVectorScale(m_Forward, distance));

    // Upベクトルを再計算
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