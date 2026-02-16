#pragma once

#include <string>
#include <memory>

// Forward declare IRenderPass to reduce include dependencies if possible,
// but since we are returning a std::shared_ptr, the full definition is needed.
#include "Renderer/Pass/IRenderPass.h"

class PassFactory
{
public:
    /**
     * @brief Creates a render pass instance from a string identifier.
     * @param passName The unique name of the pass to create.
     * @return A shared_ptr to the created IRenderPass, or nullptr if the name is not recognized.
     */
    static std::shared_ptr<IRenderPass> CreatePass(const std::string& passName);
};