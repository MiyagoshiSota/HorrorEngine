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

    void init();
    void update();
    void reset();

    std::shared_ptr<ConstantBuffer> get_constant_buffer() const { return m_TimeConstantBuffer; }

    float get_delta_time() const { return m_DeltaTime; }
    float get_total_time() const { return m_TotalTime; }

private:

    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_DeltaTime;
    float m_TotalTime;

    std::shared_ptr<ConstantBuffer> m_TimeConstantBuffer;
};