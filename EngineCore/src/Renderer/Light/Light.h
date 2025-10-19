#pragma once
#include <DirectXMath.h>

enum class LightType : int
{
    Directional,
    Point,
};

struct Light
{
    // ライトのタイプ
    LightType Type = LightType::Directional;
    // ライトの色
    DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };
    // ライトの強さ
    float Intensity = 1.0f;
};

struct PointLight : Light
{
    // ライトの位置
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    // 点光源の影響範囲
    float Range = 100.0f;
    // 点光源の距離による減衰率
    float Attenuation = 0.1f;
};

struct DirectionalLight : Light
{
    // ライトの方向（単位ベクトル）
    DirectX::XMFLOAT3 Direction = { -1.0f, -1.0f, -1.0f };
};
