#pragma once
#define NOMINMAX
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include "Renderer/StandardShader/Struct/SharedStruct.h"

struct SharedStruct::Mesh;
struct SharedStruct::Vertex;

struct aiMesh;
struct aiMaterial;

struct ImportSettings // インポートするときのパラメータ
{
    const wchar_t* filename = nullptr; // ファイルパス
    std::vector<SharedStruct::Mesh>& meshes; // 出力先のメッシュ配列
    bool inverseU = false; // U座標を反転させるか
    bool inverseV = false; // V座標を反転させるか
};

class AssimpLoader
{
public:
    bool Load(ImportSettings setting); // モデルをロードする

private:
    void LoadMesh(SharedStruct::Mesh& dst, const aiMesh* src, bool inverseU, bool inverseV);
    void LoadTexture(const wchar_t* filename, SharedStruct::Mesh& dst, const aiMaterial* src);
};

