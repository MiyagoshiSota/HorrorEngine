#pragma once

#include <memory>

#include "nlohmann/json.hpp"

class GameObject;

class Component {
public:
    virtual ~Component() = default;
    std::shared_ptr<GameObject> gameObject = nullptr; // 親オブジェクトへのポインタ

    virtual void start() {}
    virtual void update(float deltaTime) {}
    virtual void initialize(std::shared_ptr<GameObject> game_object)
    {
        gameObject = game_object;
    } // TODO:Contextとして渡すべき

    // JSONからデータを読み込むための仮想関数
    virtual void deserialize(const nlohmann::json& jsonData) {}

    // コンポーネントの型名を取得する関数
    virtual std::string get_type() = 0;

    // GUI描画用の仮想関数
    virtual void on_gui() = 0;
};
