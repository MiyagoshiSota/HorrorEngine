#pragma once
#include <d3d12.h>
#include <Renderer/StandardShader/Struct/SharedStruct.h>
#include <Renderer/Graphics/Buffer/ConstantBuffer.h>
#include <Scene/GameObject/Model/Model.h>
#include <Scene/GameObject/Mesh/Mesh.h>

#include "Component/Component.h"
#include "Renderer/Engine.h"

class GameObject
{
public:
    GameObject() {
        m_Transform = DirectX::XMMatrixIdentity();

        // コンストラクタでフレーム数分のバッファを全て作成する
        m_ConstantBuffers.resize(g_Engine->FRAME_BUFFER_COUNT);
        for (int i = 0; i < g_Engine->FRAME_BUFFER_COUNT; ++i)
        {
            m_ConstantBuffers[i] = std::make_shared<ConstantBuffer>(sizeof(SharedStruct::Transform));
        }
    };
	void init();
    void transform_update();
    void component_update();
    
    // 各種Getter
	std::string get_name() const { return name; }
    DirectX::XMMATRIX get_transform() const { return m_Transform; }
    DirectX::XMFLOAT3 get_position() const { return m_Position; }
    std::shared_ptr<Model> get_model() const { return m_Model; }
    std::shared_ptr<ConstantBuffer> get_constant_buffer() const { return  constantBuffer; }

	// 各種Setter
    void set_position(float x, float y, float z) { m_Position = { x, y, z }; }
	void set_model(std::shared_ptr<Model> model) { m_Model = model; }

    // 各種Find
    template<typename T>
    std::shared_ptr<T> find_component() {
        for (const auto& comp : components) {
            if (auto casted_comp = std::dynamic_pointer_cast<T>(comp)) {
                return casted_comp;
            }
        }
        return nullptr;
    }

public:
    /// <summary>
    /// size分のコンスタントバッファを新しく作成する
    /// </summary>
    /// <param name="size"></param>
    void create_constant_buffer(size_t size) { constantBuffer = std::make_shared<ConstantBuffer>(size); }

    // フレームインデックスを受け取るように変更
    std::shared_ptr<ConstantBuffer> get_constant_buffer(UINT frameIndex) const {
        return m_ConstantBuffers[frameIndex];
    }

public:
    std::string name;
	std::vector<std::shared_ptr<Component>> components;

private:
    std::shared_ptr<Model> m_Model;

    std::shared_ptr<ConstantBuffer> constantBuffer = nullptr;
    std::vector<std::shared_ptr<ConstantBuffer>> m_ConstantBuffers;

    DirectX::XMMATRIX m_Transform;
    DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f }; 
};
