#pragma once
#include <chrono>
#include "Renderer/Graphics/Buffer/ConstantBuffer.h"
#include "Renderer/StandardShader/Struct/SharedStruct.h" // 時間用構造体を定義する場所

class TimeManager
{
public:
    TimeManager()
    {
		m_DeltaTime = 0.0f;
		m_TotalTime = 0.0f;
    };

    void Init();
    void Update();
    void Reset();

    std::shared_ptr<ConstantBuffer> GetConstantBuffer() const { return m_TimeConstantBuffer; }

    float GetDeltaTime() const { return m_DeltaTime; }
    float GetTotalTime() const { return m_TotalTime; }

private:

    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_DeltaTime;
    float m_TotalTime;

    std::shared_ptr<ConstantBuffer> m_TimeConstantBuffer;
};