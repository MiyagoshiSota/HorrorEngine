#pragma once
#include "IReward.h"

class Work;

class StartWorkReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override;

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override;
#endif // BUILD_STANDALONE

    Work* GetWork();
    void SetWork(Work* work);

    void SetPendingWorkName(const std::string& name) { m_pendingWorkName = name; }
    void ResolvePendingWork();

    std::string GetName() const override { return "StartWorkReward"; }

private:
    Work* m_Work = nullptr;
    std::string m_pendingWorkName;
};
