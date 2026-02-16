#pragma once
#include <string>
#include <vector>

class TriggerComponent;

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

    void Update();
    bool IsComplete() const { return m_isComplete; }
    void Reset();

private:
    int m_currentTaskIndex = 0;
    bool m_isComplete = false;
};