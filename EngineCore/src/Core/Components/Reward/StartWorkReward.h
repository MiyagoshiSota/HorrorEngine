#pragma once
#include "IReward.h"
#include "Core/Components/Work/Work.h"
#include "Core/Components/Work/WorkManager.h"

class StartWorkReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        if (m_Work != nullptr)
        {
            m_Work->m_isActive = true;
            return;
        }
        printf("スタートするWorkが設定されていません。\n");
    }

    void DrawInspectorUI() override
    {
        // Workを一覧で表示して選択できるようにする
        if (ImGui::TreeNode("Work Selection"))
        {
            // ここにWorkの一覧を表示するコード
            const auto& works = WorkManager::GetInstance().GetAllWorks();
            for (const auto& work : works)
            {
                if (ImGui::Selectable(work->m_name.c_str(), m_Work == work.get()))
                {
                    m_Work = work.get();
                }
            }
            ImGui::TreePop();
        }
    }

    Work* GetWork()
    {
        return m_Work;
    }

    std::string GetName() const override
    {
        return "StartWorkReward";
    }

private:
    Work* m_Work = nullptr;
};
