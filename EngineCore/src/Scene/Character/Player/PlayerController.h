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
        const auto& inputDevice = InputDevice::GetInstance();
        
        DirectX::XMFLOAT3 direction = { 0.0f, 0.0f,0.0f };
        
        if (inputDevice.IsKeyDown('W'))
        {
            direction.z = -1.0f;
        }
        if (inputDevice.IsKeyDown('S'))
        {
            direction.z = 1.0f;
        }
        if (inputDevice.IsKeyDown('A'))
        {
            direction.x = -1.0f;
        }
        if (inputDevice.IsKeyDown('D'))
        {
            direction.x = 1.0f;
        }

        // 移動
        if (direction.x != 0.0f || direction.z != 0.0f)
        {
            auto rb = gameObject->find_component<Rigidbody>();
            if (rb && rb->GetRigidbody())
            {
                // Rigidbodyが存在する場合は、Rigidbodyを介して移動
                reactphysics3d::Vector3 currentPos = rb->GetRigidbody()->getTransform().getPosition();
                currentPos.x += direction.x * m_moveSpeed * deltaTime;
                currentPos.z += direction.z * m_moveSpeed * deltaTime;
                
                reactphysics3d::Transform new_transform = rb->GetRigidbody()->getTransform();
                new_transform.setPosition(currentPos);
                
                rb->GetRigidbody()->setTransform(new_transform);
            }
            else
            {
                printf("Rigidbodyコンポーネントが見つかりません。\n");
            }
        }
    };
    
    void Initialize(std::shared_ptr<GameObject> game_object) override
    {
        Component::Initialize(game_object);
    }

    void Deserialize(const nlohmann::json& jsonData) override
    {
        auto move_speed = ConstGameObjectSaveParamPref::kPlayerControllerMoveSpeed;
        // 移動速度の読み込み
        if (jsonData.contains(move_speed) && jsonData[move_speed].is_number())
        {
            m_moveSpeed = jsonData[move_speed].get<float>();
        }   
    };
    
    std::string GetType() override
    {
        return ConstGameObjectSaveParamPref::kComponentPlayerController;
    }
    
    void OnGui() override
    {
        // GUIで移動速度を調整可能にする
        ImGui::SliderFloat("Move Speed", &m_moveSpeed, 0.1f, 100.0f);
    };

    float GetMoveSpeed() const { return m_moveSpeed; }

private:
    float m_moveSpeed = 5.0f;
};
