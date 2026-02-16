#include "Core/Components/TriggerComponent.h"
#include "WorkFlow.h"

void WorkFlow::Update()
{
    if (m_isComplete) return;

    if (m_mode == EWorkFlowMode::Sequential)
    {
        while (m_currentTaskIndex < static_cast<int>(m_tasks.size()))
        {
            TriggerComponent* currentTask = m_tasks[m_currentTaskIndex];
            if (!currentTask)
            {
                ++m_currentTaskIndex;
                continue;
            }
            currentTask->Activate();
            if (currentTask->IsCompleted())
            {
                ++m_currentTaskIndex;
                continue;
            }
            break;
        }
        if (m_currentTaskIndex >= static_cast<int>(m_tasks.size()))
            m_isComplete = true;
    }
    else
    {
        bool allDone = true;
        for (auto task : m_tasks)
        {
            if (!task) continue;
            task->Activate();
            if (!task->IsCompleted())
                allDone = false;
        }
        m_isComplete = allDone;
    }
}

void WorkFlow::Reset()
{
    m_isComplete = false;
    m_currentTaskIndex = 0;
    for (auto task : m_tasks)
    {
        if (task)
            task->ResetTask();
    }
}
