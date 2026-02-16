#include "StartWorkReward.h"
#include "Core/Components/Work/Work.h"
#include "Core/Components/Work/WorkManager.h"

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif

void StartWorkReward::Execute(const TriggerContext& context)
{
    (void)context;
    if (m_Work != nullptr)
    {
        m_Work->m_isActive = true;
        return;
    }
    printf("スタートするWorkが設定されていません。\n");
}

#ifndef BUILD_STANDALONE
void StartWorkReward::DrawInspectorUI()
{
    if (ImGui::TreeNode("Work Selection"))
    {
        const auto& works = WorkManager::GetInstance().GetAllWorks();
        for (const auto& work : works)
        {
            if (ImGui::Selectable(work->m_name.c_str(), m_Work == work.get()))
                m_Work = work.get();
        }
        ImGui::TreePop();
    }
}
#endif

Work* StartWorkReward::GetWork()
{
    return m_Work;
}

void StartWorkReward::SetWork(Work* work)
{
    m_Work = work;
}

void StartWorkReward::ResolvePendingWork()
{
    if (m_pendingWorkName.empty()) return;
    for (const auto& w : WorkManager::GetInstance().GetAllWorks())
    {
        if (w && w->m_name == m_pendingWorkName)
        {
            m_Work = w.get();
            break;
        }
    }
    m_pendingWorkName.clear();
}
