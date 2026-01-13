#pragma once
#include <d3d12.h>
#include <Renderer/StandardShader/Struct/SharedStruct.h>
#include <Renderer/Graphics/Buffer/ConstantBuffer.h>
#include <Scene/GameObject/Mesh/Mesh.h>

#include "Component/Component.h"
#include "Renderer/Engine.h"

class GameObject
{
public:
    GameObject() {
        m_Transform = DirectX::XMMatrixIdentity();

        // コンストラクタでフレーム数分のバッファを全て作成する
        m_ConstantBuffers.resize(g_Engine->FRAME_BUFFER_COUNT + 1);
		m_ShadowConstantBuffers.resize(g_Engine->FRAME_BUFFER_COUNT + 1);
        for (int i = 0; i < g_Engine->FRAME_BUFFER_COUNT; ++i)
        {
            // TODO:野生のConstantBufferを作ってる,Heapで管理したい感ある
            m_ConstantBuffers[i] = std::make_shared<ConstantBuffer>(sizeof(SharedStruct::Transform));
			m_ShadowConstantBuffers[i] = std::make_shared<ConstantBuffer>(sizeof(SharedStruct::Transform));
			//m_ConstantBuffers[i] = std::make_shared<ConstantBuffer>(sizeof(SharedStruct::CascadedShadowMapTransform));
			//m_ShadowConstantBuffers[i] = std::make_shared<ConstantBuffer>(sizeof(SharedStruct::CascadedShadowMapTransform));
        }
    };
	void init();
    void transform_update();
    void component_update(float delta_time) const;
    
    // 各種Getter
	std::string get_name() const { return name; }
    DirectX::XMMATRIX get_transform() const { return m_Transform; }
    DirectX::XMFLOAT3 get_position() const { return m_Position; }
	DirectX::XMFLOAT3 get_rotation() const { return m_Rotation; }
	DirectX::XMFLOAT3 get_scale() const { return m_Scale; }
    std::shared_ptr<ConstantBuffer> get_constant_buffer() const { return  constantBuffer; }

	// 各種Setter
    void set_position(float x, float y, float z) { m_Position = { x, y, z }; }
	void set_rotation(float x, float y, float z) { m_Rotation = { x, y, z }; }
	void set_scale(float x, float y, float z) { m_Scale = { x, y, z }; }
	
    // 各種Find
    template<typename T>
    T* find_component() {
        for (const auto& comp : components) {
            if (auto casted_comp = dynamic_cast<T*>(comp.get())) {
                return casted_comp;
            }
        }
        return nullptr;
    }

    template<typename T>
    std::vector<T*> find_components() {
        std::vector<T*> foundComponents;
		for (const auto& comp : components) {
			T* castedComponent = dynamic_cast<T*>(comp.get());

			if (castedComponent != nullptr) {
				foundComponents.push_back(castedComponent);
			}
		}
        return foundComponents;
    }

    // コンポーネントの削除
    void RemoveComponent(Component* compToRemove)
	{
	    if (compToRemove == nullptr) return;

	    // std::remove_if を使って、条件に一致する要素を検索します
	    const auto it = std::remove_if(components.begin(), components.end(),
        
            // ラムダ式で比較条件を定義します
            [compToRemove](const std::unique_ptr<Component>& comp_ptr) 
            {
                // compPtr の .get() が、削除したいポインタと一致するかどうかを返します
                return comp_ptr.get() == compToRemove;
            }
        );

	    // 見つかった要素を削除
	    if (it != components.end())
	    {
	        components.erase(it, components.end());
	    }
	}
 
    template<typename T>
    T* AddComponent()
	{
        std::unique_ptr<T> newComponent = std::make_unique<T>();

		// Moveする前にptrをget
		T* new_component_ptr = newComponent.get();
	    components.emplace_back(std::move(newComponent));
        return new_component_ptr;
	}

public:
    /// <summary>
    /// size分のコンスタントバッファを新しく作成する
    /// </summary>
    /// <param name="size"></param>
    void create_constant_buffer(size_t size) { constantBuffer = std::make_shared<ConstantBuffer>(size); }

    std::shared_ptr<ConstantBuffer> get_constant_buffer(UINT frameIndex) const {
        return m_ConstantBuffers[frameIndex];
    }

    std::shared_ptr<ConstantBuffer> get_shadow_constant_buffer(UINT frameIndex) const {
        return m_ShadowConstantBuffers[frameIndex];
	}

public:
    std::string name;
	std::vector<std::unique_ptr<Component>> components;

private:
    std::shared_ptr<ConstantBuffer> constantBuffer = nullptr;
    std::vector<std::shared_ptr<ConstantBuffer>> m_ConstantBuffers;
    std::vector<std::shared_ptr<ConstantBuffer>> m_ShadowConstantBuffers;

    DirectX::XMMATRIX m_Transform;
	DirectX::XMFLOAT3 m_Rotation = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_Scale = { 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f }; 
};
