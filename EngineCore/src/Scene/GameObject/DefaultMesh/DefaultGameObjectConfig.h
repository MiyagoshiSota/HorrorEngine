#pragma once

#include <string>
#include <DirectXMath.h>
#include "Modules/PublicConst/const_premitive_gameobject_pref.h"

namespace DefaultGameObjectConfig
{
    // デフォルトテクスチャパス
    namespace TexturePaths
    {
        static inline const wchar_t* DefaultWhiteDiffuse = L"assets/DefaultTexture/default_diffuse_white.png";
    }

    // デフォルトマテリアル設定
    namespace MaterialDefaults
    {
        static inline const DirectX::XMFLOAT4 DefaultAlbedoFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        static inline const float DefaultMetallicFactor = 1.0f;
        static inline const float DefaultRoughnessFactor = 1.0f;
    }

    // デフォルトGameObjectの設定構造体
    struct DefaultGameObjectSettings
    {
        const char* Name;
        const wchar_t* AlbedoTexturePath;
        DirectX::XMFLOAT4 AlbedoFactor;
        float MetallicFactor;
        float RoughnessFactor;
    };

    // Quad設定
    static inline const DefaultGameObjectSettings QuadSettings = {
        const_premitive_gameobject_pref::QuadGameObjectKey,
        TexturePaths::DefaultWhiteDiffuse,
        MaterialDefaults::DefaultAlbedoFactor,
        MaterialDefaults::DefaultMetallicFactor,
        MaterialDefaults::DefaultRoughnessFactor
    };

    // Cube設定
    static inline const DefaultGameObjectSettings CubeSettings = {
        const_premitive_gameobject_pref::CubeGameObjectKey,
        TexturePaths::DefaultWhiteDiffuse,
        MaterialDefaults::DefaultAlbedoFactor,
        MaterialDefaults::DefaultMetallicFactor,
        MaterialDefaults::DefaultRoughnessFactor
    };
}
