#pragma once
#include <string>
#include <cstring>
#include "IReward.h"
#include "Core/Components/TriggerContext/TriggerContext.h"

#ifndef BUILD_STANDALONE
#include "imgui.h"
#endif // BUILD_STANDALONE

class PrintHelloReward : public IReward
{
public:
    void Execute(const TriggerContext& context) override
    {
        (void)context;
        printf("%s\n", m_message.c_str());
    }

#ifndef BUILD_STANDALONE
    void DrawInspectorUI() override
    {
        char buffer[256] = {};
        strncpy(buffer, m_message.c_str(), 255);
        buffer[255] = '\0';
        if (ImGui::InputText("Message", buffer, sizeof(buffer)))
        {
            m_message = buffer;
        }
    }
#endif // BUILD_STANDALONE

    std::string GetName() const override
    {
        return "PrintHelloAction";
    }

private:
    std::string m_message = "hello";
};
