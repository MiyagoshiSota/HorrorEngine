#pragma once
#include <string>
#include <vector>
#include "Core/Components/TriggerComponent.h" // TriggerComponentの定義をインクルード

// 実行モード
enum class EWorkFlowMode
{
    Sequential, // 順序実行(0番目から順に実行)
    Parallel    // 任意実行(全て同時に実行)
};

class WorkFlow
{
public:
    std::string m_name;
    EWorkFlowMode m_mode = EWorkFlowMode::Sequential;
    std::vector<std::shared_ptr<TriggerComponent>> m_tasks; // シーン内のTriggerComponentへのポインタを保持

    WorkFlow(std::string name) : m_name(std::move(name)), m_currentTaskIndex(0), m_isComplete(false) {}

    // ワークフローの更新処理
    void Update()
    {
        if (m_isComplete) return;

        if (m_mode == EWorkFlowMode::Sequential)
        {
            // シーケンシャルモード
            if (m_currentTaskIndex < m_tasks.size())
            {
                std::shared_ptr<TriggerComponent> currentTask = m_tasks[m_currentTaskIndex];
                
                // 現在のタスクをアクティベート
                currentTask->Activate(); 

                // タスクが完了したら次のタスクへ
                if (currentTask->IsCompleted())
                {
                    m_currentTaskIndex++;
                }
            }
            else
            {
                // 全てのタスクが完了
                m_isComplete = true;
            }
        }
        else // Parallelモード
        {
            bool allDone = true;
            for (auto task : m_tasks)
            {
                task->Activate(); // 全てのタスクを最初からアクティベート
                if (!task->IsCompleted())
                {
                    allDone = false;
                }
            }
            m_isComplete = allDone;
        }
    }

    bool IsComplete() const { return m_isComplete; }

    void Reset()
    {
        m_isComplete = false;
        m_currentTaskIndex = 0;
        for (auto task : m_tasks)
        {
            task->ResetTask();
        }
    }

private:
    int m_currentTaskIndex = 0;
    bool m_isComplete = false;
};