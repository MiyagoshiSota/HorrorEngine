#pragma once
#include <DirectXMath.h>
#include <memory>
#include <string>

struct PointLight;

enum class LightType : int
{
    Directional,
    Point,
    Spot
};

struct Light
{
    // ライトのタイプ
    LightType Type = LightType::Directional;
    // ライトの色
    DirectX::XMFLOAT3 Color = { 1.0f, 1.0f, 1.0f };
    // ライトの強さ
    float Intensity = 1.0f;

    // ライトの色を取得
    void set_color(const DirectX::XMFLOAT3& color)
    {
        Color = color;
    }
    
    // ライトのタイプを文字列で取得
    std::string get_type_string() const
    {
        switch (Type)
        {
        case LightType::Directional:
            return "Directional";
        case LightType::Point:
            return "Point";
        case LightType::Spot:
            return "Spot";
        default:
            return "Unknown";
        }
    }

    virtual void set_position(float x, float y, float z) = 0;
};

struct PointLight : Light
{
    // ライトの位置
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    // 点光源の影響範囲
    float Range = 100.0f;
    // 点光源の距離による減衰率
    float Attenuation = 0.1f;

    void set_position(float x, float y, float z) override
    {
        Position = { x, y, z };
    }
};

struct DirectionalLight : Light
{
    // ライトの方向（単位ベクトル）
    DirectX::XMFLOAT3 Direction = { -1.0f, -1.0f, -1.0f };

    void set_position(float x, float y, float z) override
    {
        // ディレクショナルライトの場合、位置は方向ベクトルとして扱う
        Direction = { x, y, z };
    }
};

struct SpotLight: Light
{
    // ライトの位置
    DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
    // ライトの方向（単位ベクトル）
    DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
    // スポットライトの内角度（ラジアン）
    float InnerAngle = 0.5f; // 約28.65度
    // スポットライトの外角度（ラジアン）
    float OuterAngle = 0.75f; // 約42.97度

    // スポットの影響範囲
    float Range = 100.0f;
    // スポットの距離による減衰率
    float Attenuation = 0.1f;
    
    void set_position(float x, float y, float z) override
    {
        Position = { x, y, z };
    }
};
