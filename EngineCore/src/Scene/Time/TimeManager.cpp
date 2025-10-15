#include "TimeManager.h"

void TimeManager::init()
{
    m_TimeConstantBuffer = std::make_unique<ConstantBuffer>(sizeof(SharedStruct::TimeData));
    m_LastFrameTime = std::chrono::steady_clock::now();
}

void TimeManager::update()
{
    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<float> deltaDuration = currentTime - m_LastFrameTime;

    m_DeltaTime = deltaDuration.count();
    m_TotalTime += m_DeltaTime;

    m_LastFrameTime = currentTime;

    auto* timeData = m_TimeConstantBuffer->GetPtr<SharedStruct::TimeData>();
    timeData->DeltaTime = m_DeltaTime;
    timeData->TotalTime = m_TotalTime;
}

void TimeManager::reset()
{
	m_DeltaTime = 0.0f;
	m_TotalTime = 0.0f;
	m_LastFrameTime = std::chrono::steady_clock::now();
}
