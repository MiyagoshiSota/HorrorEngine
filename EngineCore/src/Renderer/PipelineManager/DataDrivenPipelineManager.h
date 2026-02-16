#pragma once

#include "Renderer/PipelineManager/IPipelineManager.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations
class IRenderPass;
class SettingStore;
class ITargetBase;
class RenderContext;

/**
 * @class DataDrivenPipelineManager
 * @brief An implementation of IPipelineManager that builds and executes a rendering pipeline
 * based on an external definition file (e.g., JSON).
 *
 * This manager is responsible for:
 * 1. Parsing a pipeline definition file to understand the required passes and their order.
 * 2. Using a PassFactory to create instances of these passes.
 * 3. Managing the lifecycle of render targets required by the pipeline.
 * 4. Executing the passes in the correct order each frame.
 * 5. Checking a SettingStore to dynamically enable or disable passes.
 * 6. Providing access to specific passes for runtime parameter configuration.
 */
class DataDrivenPipelineManager : public IPipelineManager
{
public:
    /**
     * @brief Constructs the pipeline manager.
     * @param pipelineDefinitionPath Path to the JSON file defining the pipeline.
     * @param settings A shared pointer to the SettingStore for dynamic configuration.
     */
    DataDrivenPipelineManager(const std::string& pipelineDefinitionPath, std::shared_ptr<SettingStore> settings);
    ~DataDrivenPipelineManager() override = default;

    /**
     * @brief Executes the entire rendering pipeline for one frame.
     */
    void Execute() override;

    /**
     * @brief Retrieves a specific pass by its instance name.
     * This allows external systems (like UI) to access and configure pass-specific parameters.
     * @tparam T The concrete type of the pass to retrieve.
     * @param name The instance name of the pass as defined in the pipeline file.
     * @return A shared_ptr to the pass of type T, or nullptr if not found or type mismatch.
     */
    template<typename T>
    std::shared_ptr<T> GetPass(const std::string& name) const;

    /**
     * @brief Retrieves a render target by its name for previewing or debugging.
     * @param name The name of the render target as defined in the pipeline file.
     * @return A shared_ptr to the render target, or nullptr if not found.
     */
    std::shared_ptr<ITargetBase> GetRenderTargetForPreview(const std::string& name) const;

    /**
     * @brief Gets the underlying setting store.
     * @return A shared pointer to the SettingStore.
     */
    std::shared_ptr<SettingStore> GetSettingStore() const { return m_settings; }

private:
    // Represents a single step in the parsed rendering pipeline.
    struct PipelinePass
    {
        std::string name;                      // Instance name, e.g., "MainShadows"
        std::shared_ptr<IRenderPass> pass;     // The actual pass object
        std::string enabled_by_key;            // Key in SettingStore to check for enablement
        std::vector<std::string> inputs;       // Names of input render targets
        std::vector<std::string> outputs;      // Names of output render targets
    };

    // Private methods for initialization
    void LoadPipelineDefinition(const std::string& path);
    void CreateRenderTargets(const nlohmann::json& rtDefs);
    void CreatePasses(const nlohmann::json& passDefs);

private:
    std::shared_ptr<SettingStore> m_settings;
    std::vector<PipelinePass> m_passes;
    std::unordered_map<std::string, std::shared_ptr<IRenderPass>> m_passMap; // For quick name-based access
    std::unordered_map<std::string, std::shared_ptr<ITargetBase>> m_renderTargets;
    std::unique_ptr<class TempRenderTargetPool> m_tempRenderTargetPool;
};

template<typename T>
std::shared_ptr<T> DataDrivenPipelineManager::GetPass(const std::string& name) const
{
    auto it = m_passMap.find(name);
    if (it != m_passMap.end())
    {
        return std::dynamic_pointer_cast<T>(it->second);
    }
    return nullptr;
}