#pragma once
#include <DirectXMath.h>

enum class LightType : int
{
    Directional,
    Point,
};

struct Light
{
    // --- 共通のプロパティ ---
    LightType Type = LightType::Directional;
    DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f }; // ライトの色
    float Intensity = 1.0f; // ライトの強さ

    // --- 種類ごとのプロパティ ---
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f }; // 指向性ライトの向き
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f }; // 点光源の位置
    float Range = 100.0f; // 点光源の影響範囲
    float Attenuation = 0.1f; // 点光源の距離による減衰率
};
