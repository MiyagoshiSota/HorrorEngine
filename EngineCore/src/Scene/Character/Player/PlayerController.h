#pragma once
#include "Input/InputDevice.h"
#include "Scene/GameObject/GameObject.h"
#include "Scene/GameObject/Component/Component.h"

class PLayerController : public Component
{
public:
    ~PLayerController() override = default;

    void start() override{};
    
    void update(float deltaTime) override
    {
        const auto& m_input_device_ = InputDevice::GetInstance();
        
        // プレイヤーの移動処理をここに実装

        DirectX::XMFLOAT3 direction = { 0.0f, 0.0f,0.0f };
        
        if (m_input_device_.IsKeyDown('W'))
        {
            direction.z = -1.0f;
        }
        if (m_input_device_.IsKeyDown('S'))
        {
            direction.z = 1.0f;
        }
        if (m_input_device_.IsKeyDown('A'))
        {
            direction.x = -1.0f;
        }
        if (m_input_device_.IsKeyDown('D'))
        {
            direction.x = 1.0f;
        }

        if (direction.x != 0.0f || direction.z != 0.0f)
        {
            // 位置の更新

            auto rb = gameObject->find_component<Rigidbody>();
            if (rb && rb->get_rigidbody())
            {
                // Rigidbodyが存在する場合は、Rigidbodyを介して移動
                reactphysics3d::Vector3 currentPos = rb->get_rigidbody()->getTransform().getPosition();
                currentPos.x += direction.x * m_moveSpeed * deltaTime;
                currentPos.z += direction.z * m_moveSpeed * deltaTime;
                
                reactphysics3d::Transform new_transform = rb->get_rigidbody()->getTransform();
                new_transform.setPosition(currentPos);
                
                rb->get_rigidbody()->setTransform(new_transform);
            }
            else
            {
                printf("Rigidbodyコンポーネントが見つかりません。\n");
            }
        }
    };
    
    void initialize(std::shared_ptr<GameObject> game_object) override
    {
        Component::initialize(game_object);
    }

    void deserialize(const nlohmann::json& jsonData) override
    {
        auto move_speed = const_gameobject_save_param_pref::PlayerControllerMoveSpeed;
        // 移動速度の読み込み
        if (jsonData.contains(move_speed) && jsonData[move_speed].is_number())
        {
            m_moveSpeed = jsonData[move_speed].get<float>();
        }   
    };
    
    std::string get_type() override
    {
        return const_gameobject_save_param_pref::ComponentPlayerController;
    }
    
    void on_gui() override
    {
        // GUIで移動速度を調整可能にする
        ImGui::SliderFloat("Move Speed", &m_moveSpeed, 0.1f, 100.0f);
    };

    float get_move_speed() const { return m_moveSpeed; }

private:
    float m_moveSpeed = 5.0f;
};
