#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Core/App.h"
#include "Core/Components/WorkFlow/WorkFlow.h"
#include "Core/Components/Trigger/ITriggerCondition.h"

class Work
{
public:
    std::string m_name;
    std::vector<std::unique_ptr<WorkFlow>> m_workflows;

    TriggerContext m_context;
    
    // このWorkを開始するためのトリガー
    std::unique_ptr<ITriggerCondition> m_startCondition;

    // このWorkを完了させるための報酬
    std::vector<std::unique_ptr<IAction>> m_rewardActions;

    bool m_isActive = false;
    bool m_isComplete = false;
    int m_currentWorkflowIndex = 0;

public:
    Work(std::string name) : m_name(std::move(name))
    {
        m_context.m_Owner = nullptr;
    }

    void Update()
    {
        if (m_isComplete) return;

        // まだアクティブでなければ、開始条件をチェック
        if (!m_isActive)
        {
            if (m_startCondition && m_startCondition->Check(m_context))
            {
                m_isActive = true;
                // 最初のWorkFlowのタスクをリセット
                if (!m_workflows.empty())
                {
                    m_workflows[0]->Reset();
                }
            }
        }

        // アクティブなら、現在のWorkFlowを更新
        if (m_isActive)
        {
            if (m_currentWorkflowIndex < m_workflows.size())
            {
                auto currentWorkflow = std::move(m_workflows[m_currentWorkflowIndex]);
                currentWorkflow->Update();

                // 現在のWorkFlowが完了したら次へ
                if (currentWorkflow->IsComplete())
                {
                    m_currentWorkflowIndex++;
                    // 次のWorkFlowがあればリセット
                    if (m_currentWorkflowIndex < m_workflows.size())
                    {
                        m_workflows[m_currentWorkflowIndex]->Reset();
                    }
                }
            }
            else
            {
                // 全てのWorkFlowが完了
                m_isComplete = true;
                // 完了時の報酬アクションを実行
                for (const auto& action : m_rewardActions)
                {
                    if (action)
                    {
                        action->Execute(m_context);
                    }
                }
            }
        }
    }

    void Reset()
    {
        m_isActive = false;
        m_isComplete = false;
        m_currentWorkflowIndex = 0;
        for (auto& wf : m_workflows)
        {
            wf->Reset();
        }
    }
};