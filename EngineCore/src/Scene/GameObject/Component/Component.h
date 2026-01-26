#pragma once

#include <memory>

#include <nlohmann/json.hpp>

class GameObject;

class Component {
public:
    virtual ~Component() = default;
    std::shared_ptr<GameObject> gameObject = nullptr; // 親オブジェクトへのポインタ

    virtual void start() {}
    virtual void update(float deltaTime) {}
    virtual void Initialize(std::shared_ptr<GameObject> game_object)
    {
        gameObject = game_object;
    } // TODO:Contextとして渡すべき

    // JSONからデータを読み込むための仮想関数
    virtual void Deserialize(const nlohmann::json& jsonData) {}

    // コンポーネントの型名を取得する関数
    virtual std::string GetType() = 0;

#ifndef BUILD_STANDALONE
    // GUI描画用の仮想関数
    virtual void OnGui() = 0;
#else
    // BUILD_STANDALONE時は空実装
    virtual void OnGui() {}
#endif // BUILD_STANDALONE
};
