#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <cstdint>
#include "Modules/ComPtr.h"

#include "Renderer/Pass/IRenderPass.h"

struct Particle
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 velocity;
    float life;
    float padding;
};

struct SceneConstants
{
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
    DirectX::XMFLOAT3 cameraPos;
    float rainLength;
};

struct FrameConstants
{
    float deltaTime;
    DirectX::XMFLOAT3 windForce;

    DirectX::XMFLOAT3 emitCenter;
    float emitRadius;

    float emitHeight;
    float groundHeight;
    float initialLifeMin;
    float initialLifeMax;
};

class RainParticleSystem : IRenderPass
{
public:
    RainParticleSystem();
    ~RainParticleSystem() override {};

    void Execute(RenderContext& context) override;
    void Draw(RenderContext& context);

public:
    // パーティクルの最大数
    static const uint32_t MAX_PARTICLES = 100000;
    // スレッドグループのサイズ (HLSL側と一致)
    static const uint32_t THREAD_GROUP_SIZE = 256;

    // 定数バッファ
    std::shared_ptr<ConstantBuffer> constantBuffer;
    
    // パーティクルデータバッファ
    ComPtr<ID3D12Resource> m_pParticleBuffer;
    ComPtr<ID3D12Resource> m_pUploadBuffer;

    // ディスクリプタハンドル
    std::shared_ptr<DescriptorHandle> srvHandle;
    std::shared_ptr<DescriptorHandle> uavHandle;

public:
    // シーン定数バッファ
    std::shared_ptr<ConstantBuffer> sceneConstantBuffer;

private:
    bool firstFrame = true;
};
