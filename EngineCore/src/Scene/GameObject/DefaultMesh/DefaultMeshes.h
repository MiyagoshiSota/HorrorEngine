#pragma once

#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include "DefaultGameObjectConfig.h"

namespace DefaultMeshes
{
    // 四角形のメッシュデータを返す関数
    inline SharedStruct::Mesh create_quad()
    {
        SharedStruct::Mesh quad;

        // 頂点データ
        quad.Vertices = {
            //   位置                  法線                  UV座標          接空間     色
            {{ -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, {0,0,0}, {1,1,1,1} },
            {{  1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, {0,0,0}, {1,1,1,1} },
            {{ 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, {0,0,0}, {1,1,1,1} },
            {{  1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, {0,0,0}, {1,1,1,1} }
        };

        // インデックスデータ
        quad.Indeices = { 0, 2, 1, 1, 2, 3 };

        // デフォルトのテクスチャとマテリアル設定
        const auto& settings = DefaultGameObjectConfig::QuadSettings;
        quad.hAlbedoMap = settings.AlbedoTexturePath;
        quad.albedoFactor = settings.AlbedoFactor;
        quad.metallicFactor = settings.MetallicFactor;
        quad.roughnessFactor = settings.RoughnessFactor;

        return quad;
    }

	// 立方体のメッシュデータを返す関数
    inline SharedStruct::Mesh create_cube()
    {
        SharedStruct::Mesh cube;

        // 立方体の頂点データ (各面に固有の法線とUVを持つため24頂点)
        cube.Vertices = {
            // 前面 (-Z)
            {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{-1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f,  1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {0,0,0}, {1,1,1,1}},

            // 背面 (+Z)
            {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f,  1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{-1.0f,  1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0,0,0}, {1,1,1,1}},

            // 上面 (+Y)
            {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{-1.0f, 1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f, 1.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0,0,0}, {1,1,1,1}},

            // 底面 (-Y)
            {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{-1.0f, -1.0f,  1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0,0,0}, {1,1,1,1}},

            // 左面 (-X)
            {{-1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{-1.0f,  1.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{-1.0f,  1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0,0,0}, {1,1,1,1}},

            // 右面 (+X)
            {{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f,  1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0,0,0}, {1,1,1,1}},
            {{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0,0,0}, {1,1,1,1}},
        };


        // インデックスデータ
        cube.Indeices = {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            8, 9, 10, 8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23
        };

        // デフォルトのテクスチャとマテリアル設定
        const auto& settings = DefaultGameObjectConfig::CubeSettings;
        cube.hAlbedoMap = settings.AlbedoTexturePath;
        cube.albedoFactor = settings.AlbedoFactor;
        cube.metallicFactor = settings.MetallicFactor;
        cube.roughnessFactor = settings.RoughnessFactor;

        return cube;
    }
}