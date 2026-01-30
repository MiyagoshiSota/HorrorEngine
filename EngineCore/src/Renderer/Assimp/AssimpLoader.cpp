#include "AssimpLoader.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <d3dx12.h>
#include <filesystem>
#include <Windows.h> // MultiByteToWideChar / WideCharToMultiByte のために必要
#include <cassert> // assert のために必要

namespace fs = std::filesystem;

// ファイルパスからディレクトリ部分（"C:/path/to/"）を取得する
std::wstring GetDirectoryPath(const std::wstring& origin)
{
    fs::path p = origin.c_str();
    return p.parent_path().wstring() + L"/"; // フォルダのパス + 末尾のスラッシュ
}

// std::wstring (UTF-16) から std::string (UTF-8) へ変換
std::string ToUTF8(const std::wstring& value)
{
    if (value.empty()) {
        return std::string();
    }

    auto length = WideCharToMultiByte(CP_UTF8, 0U, value.data(), (int)value.length(), nullptr, 0, nullptr, nullptr);
    if (length == 0) {
        // エラー処理 (必要に応じて)
        return std::string();
    }

    std::string result;
    result.resize(length); // 必要なバッファを確保

    WideCharToMultiByte(CP_UTF8, 0U, value.data(), (int)value.length(), &result[0], length, nullptr, nullptr);

    return result;
}

// std::string (UTF-8) から std::wstring (UTF-16) へ変換
std::wstring ToWideString(const std::string& str)
{
    if (str.empty()) {
        return std::wstring();
    }

    auto num1 = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), nullptr, 0);
    if (num1 == 0) {
        // エラー処理
        return std::wstring();
    }

    std::wstring wstr;
    wstr.resize(num1); // 必要なバッファを確保

    auto num2 = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.length(), &wstr[0], num1);

    assert(num1 == num2); // 変換が成功したか
    return wstr;
}

bool AssimpLoader::Load(ImportSettings settings)
{
    if (settings.filename == nullptr)
    {
        return false;
    }

    auto& meshes = settings.meshes;
    auto inverseU = settings.inverseU;
    auto inverseV = settings.inverseV;

    auto path = ToUTF8(settings.filename);
    if (path.empty()) {
        printf("Failed to convert filename to UTF-8.\n");
        return false;
    }

    Assimp::Importer importer;
    int flag = 0;
    flag |= aiProcess_Triangulate;
    flag |= aiProcess_PreTransformVertices; 
    flag |= aiProcess_CalcTangentSpace;
    flag |= aiProcess_GenSmoothNormals;
    flag |= aiProcess_GenUVCoords;
    flag |= aiProcess_RemoveRedundantMaterials;
    flag |= aiProcess_OptimizeMeshes;
    flag |= aiProcess_JoinIdenticalVertices; // 頂点数を最適化

    auto scene = importer.ReadFile(path, flag);

    if (scene == nullptr)
    {
        // もし読み込みエラーがでたら表示する
        printf(importer.GetErrorString());
        printf("\n");
        return false;
    }

    // 読み込んだデータを自分で定義したMesh構造体に変換する
    meshes.clear();
    meshes.resize(scene->mNumMeshes);
    for (size_t i = 0; i < meshes.size(); ++i)
    {
        const auto pMesh = scene->mMeshes[i];
        LoadMesh(meshes[i], pMesh, inverseU, inverseV);

        // メッシュから、それが使用するマテリアルの「インデックス」を取得する
        unsigned int materialIndex = pMesh->mMaterialIndex;

        // そのインデックスを使って、シーンから正しいマテリアルを取得する
        const auto pMaterial = scene->mMaterials[materialIndex];

        // 正しいマテリアルを使ってテクスチャをロードする
        // (settings.filename を渡して、テクスチャの基底パスを解決する)
        LoadTexture(settings.filename, meshes[i], pMaterial);
    }

    return true;
}

void AssimpLoader::LoadMesh(SharedStruct::Mesh& dst, const aiMesh* src, bool inverseU, bool inverseV)
{
    aiVector3D zero3D(0.0f, 0.0f, 0.0f);
    aiColor4D zeroColor(0.0f, 0.0f, 0.0f, 0.0f);

    dst.Vertices.resize(src->mNumVertices);

    for (auto i = 0u; i < src->mNumVertices; ++i)
    {
        auto position = &(src->mVertices[i]);
        auto normal = &(src->mNormals[i]);
        auto uv = (src->HasTextureCoords(0)) ? &(src->mTextureCoords[0][i]) : &zero3D;
        auto tangent = (src->HasTangentsAndBitangents()) ? &(src->mTangents[i]) : &zero3D;
        auto color = (src->HasVertexColors(0)) ? &(src->mColors[0][i]) : &zeroColor;

        // 反転オプションがあったらUVを反転させる
        float uv_x = uv->x;
        float uv_y = uv->y;
        if (inverseU)
        {
            uv_x = 1.0f - uv_x;
        }
        if (inverseV)
        {
            uv_y = 1.0f - uv_y;
        }

        SharedStruct::Vertex vertex = {};
        // DirectX 12は左手座標系、Assimpは右手座標系のためX座標を反転
        vertex.Position = DirectX::XMFLOAT3(-position->x, position->y, position->z);
        // TODO:法線ベクトルないモデルも存在するから注意 (aiProcess_GenSmoothNormals フラグで大抵は生成される)
        // 座標系変換に伴い法線のX成分も反転
        vertex.Normal = DirectX::XMFLOAT3(-normal->x, normal->y, normal->z);
        vertex.UV = DirectX::XMFLOAT2(uv_x, uv_y);
        // 座標系変換に伴いタンジェントのX成分も反転
        vertex.Tangent = DirectX::XMFLOAT3(-tangent->x, tangent->y, tangent->z);
        vertex.Color = DirectX::XMFLOAT4(color->r, color->g, color->b, color->a);

        dst.Vertices[i] = vertex;
    }

    // !! TYPO FIX !!: "Indeices" -> "Indices"
    dst.Indeices.resize(src->mNumFaces * 3);

    for (auto i = 0u; i < src->mNumFaces; ++i)
    {
        const auto& face = src->mFaces[i];
        // aiProcess_Triangulate フラグを立てているので、mNumIndices は必ず 3
        assert(face.mNumIndices == 3);

        // DirectX 12は時計回り（CW）が前面、Assimpは反時計回り（CCW）のため順序を反転
        dst.Indeices[i * 3 + 0] = face.mIndices[0];
        dst.Indeices[i * 3 + 1] = face.mIndices[2];
        dst.Indeices[i * 3 + 2] = face.mIndices[1];
    }
}


/**
 * @brief マテリアルからPBRテクスチャをロードする
 * @param filename モデルファイルのフルパス (テクスチャの相対パスを解決するために使う)
 * @param dst      ロード結果を格納するメッシュ
 * @param src      assimpのマテリアル
*/
void AssimpLoader::LoadTexture(const wchar_t* filename, SharedStruct::Mesh& dst, const aiMaterial* src)
{
    // FBXのディレクトリパス
    auto dir = GetDirectoryPath(filename);

    // テクスチャ読み込みを試みるヘルパーラムダ
    // [引数]
    // - outPath:   (出力) 見つかったテクスチャのフルパス
    // - outHasMap: (出力) テクスチャが見つかったかどうかのフラグ
    // - texType:   (入力) assimpのテクスチャタイプ
    auto TryLoadTexture =
        [&](std::wstring& outPath, bool& outHasMap, aiTextureType texType) -> bool
    {
        aiString path;
        // 0番目のテクスチャスロットを取得
        if (src->GetTexture(texType, 0, &path) == AI_SUCCESS)
        {
            // テクスチャパスは相対パスで入っている (例: "textures/albedo.png")
            auto file = std::string(path.C_Str());
            // モデルのディレクトリパスと結合してフルパスにする
            outPath = dir + ToWideString(file);
            outHasMap = true;
            return true;
        }
        outPath.clear();
        outHasMap = false;
        return false;
    };

    // Albedo (Base Color) マップ
    if (!TryLoadTexture(dst.hAlbedoMap, dst.HasAlbedoMap, aiTextureType_BASE_COLOR))
    {
        TryLoadTexture(dst.hAlbedoMap, dst.HasAlbedoMap, aiTextureType_DIFFUSE);
    }

    // Normal マップ
    TryLoadTexture(dst.hNormalMap, dst.HasNormalMap, aiTextureType_NORMALS);

    // Metallic マップ
    TryLoadTexture(dst.hMetallicMap, dst.HasMetallicMap, aiTextureType_UNKNOWN);

    // Roughness マップ
    TryLoadTexture(dst.hRoughnessMap, dst.HasRoughnessMap, aiTextureType_DIFFUSE_ROUGHNESS);

    // Ambient Occlusion (AO) マップ
	TryLoadTexture(dst.hAOMap, dst.HasAOMap, aiTextureType_AMBIENT_OCCLUSION);

    // Emissive マップ
    TryLoadTexture(dst.hEmissiveMap, dst.HasEmissiveMap, aiTextureType_EMISSIVE);

     // Albedo Color (Base Color Factor)
     aiColor4D albedoColor(1.0f, 1.0f, 1.0f, 1.0f); // デフォルトは白
     // 1. glTF PBR のキーを試す
     if (src->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, albedoColor) != AI_SUCCESS) {
         // 2. 従来のキー (DIFFUSE) を試す
         src->Get(AI_MATKEY_COLOR_DIFFUSE, albedoColor);
     }
     dst.albedoFactor = { albedoColor.r, albedoColor.g, albedoColor.b, albedoColor.a };

     // Metallic Factor
     float metallicFactor = 1.0f; // デフォルトは 1.0
     // v5.0.1.6 の正しい定数名を使用
     src->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metallicFactor);
     dst.metallicFactor = metallicFactor;

     // Roughness Factor
     float roughnessFactor = 1.0f; // デフォルトは 1.0
     // v5.0.1.6 の正しい定数名を使用
     src->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, roughnessFactor);
     dst.roughnessFactor = roughnessFactor;
}