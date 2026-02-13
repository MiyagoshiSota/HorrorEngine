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
    std::vector<TriggerComponent*> m_tasks; // シーン内のTriggerComponentへのポインタを保持

    WorkFlow(std::string name) : m_name(std::move(name)), m_currentTaskIndex(0), m_isComplete(false) {}

    // ワークフローの更新処理
    void Update()
    {
        if (m_isComplete) return;

        if (m_mode == EWorkFlowMode::Sequential)
        {
            // シーケンシャルモード
            while (m_currentTaskIndex < static_cast<int>(m_tasks.size()))
            {
                TriggerComponent* currentTask = m_tasks[m_currentTaskIndex];

                // nullptr（Noneタスク）の場合はスキップして次へ
                if (!currentTask)
                {
                    ++m_currentTaskIndex;
                    continue;
                }
                
                // 現在のタスクをアクティベート
                currentTask->Activate(); 

                // タスクが完了したら次のタスクへ
                if (currentTask->IsCompleted())
                {
                    ++m_currentTaskIndex;
                    continue;
                }

                // まだ完了していないタスクがあるので一旦抜ける
                break;
            }

            // すべてのタスク（nullptr含む）を処理し終えたら完了
            if (m_currentTaskIndex >= static_cast<int>(m_tasks.size()))
            {
                m_isComplete = true;
            }
        }
        else // Parallelモード
        {
            bool allDone = true;
            for (auto task : m_tasks)
            {
                if (!task)
                {
                    // nullptr（Noneタスク）はスキップ
                    continue;
                }

                task->Activate(); // 実タスクのみアクティベート
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
            if (task)
            {
                task->ResetTask();
            }
        }
    }

private:
    int m_currentTaskIndex = 0;
    bool m_isComplete = false;
};