#pragma once

#include <string>
#include <DirectXMath.h>
#include "Modules/PublicConst/ConstPrimitiveGameObjectPref.h"

namespace DefaultGameObjectConfig
{
    // デフォルトテクスチャパス
    namespace TexturePaths
    {
        static inline const wchar_t* kDefaultWhiteDiffuse = L"Assets/DefaultTexture/default_diffuse_white.png";
    }

    // デフォルトマテリアル設定
    namespace MaterialDefaults
    {
        static inline const DirectX::XMFLOAT4 kDefaultAlbedoFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        static inline const float kDefaultMetallicFactor = 1.0f;
        static inline const float kDefaultRoughnessFactor = 1.0f;
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
    static inline const DefaultGameObjectSettings kQuadSettings = {
        ConstPrimitiveGameObjectPref::kQuadGameObjectKey,
        TexturePaths::kDefaultWhiteDiffuse,
        MaterialDefaults::kDefaultAlbedoFactor,
        MaterialDefaults::kDefaultMetallicFactor,
        MaterialDefaults::kDefaultRoughnessFactor
    };

    // Cube設定
    static inline const DefaultGameObjectSettings kCubeSettings = {
        ConstPrimitiveGameObjectPref::kCubeGameObjectKey,
        TexturePaths::kDefaultWhiteDiffuse,
        MaterialDefaults::kDefaultAlbedoFactor,
        MaterialDefaults::kDefaultMetallicFactor,
        MaterialDefaults::kDefaultRoughnessFactor
    };
}
