#include "DataDrivenPipelineManager.h"
#include "Renderer/Pass/PassFactory.h"
#include "System/SettingStore.h"
#include "Renderer/RenderContext/RenderContext.h"
#include "Renderer/RenderContext/TempRenderTargetPool.h"
#include "Renderer/Target/RenderTarget.h"
#include "Renderer/Target/DepthStencilTarget.h"
#include "Renderer/Graphics.h"
#include "Renderer/Engine.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

// Helper function to parse DXGI_FORMAT from string
// This should ideally be in a utility file.
DXGI_FORMAT StringToDXGIFormat(const std::string& formatStr)
{
    if (formatStr == "RGBA16_FLOAT") return DXGI_FORMAT_R16G16B16A16_FLOAT;
    if (formatStr == "RGBA8_UNORM") return DXGI_FORMAT_R8G8B8A8_UNORM;
    if (formatStr == "D32_FLOAT") return DXGI_FORMAT_D32_FLOAT;
    if (formatStr == "R32_FLOAT") return DXGI_FORMAT_R32_FLOAT;
    if (formatStr == "R16_FLOAT") return DXGI_FORMAT_R16_FLOAT;
    if (formatStr == "R8_UNORM") return DXGI_FORMAT_R8_UNORM;
    if (formatStr == "RG16_FLOAT") return DXGI_FORMAT_R16G16_FLOAT;
    // Add other formats as needed
    return DXGI_FORMAT_UNKNOWN;
}

DataDrivenPipelineManager::DataDrivenPipelineManager(const std::string& pipelineDefinitionPath, std::shared_ptr<SettingStore> settings)
    : m_settings(std::move(settings))
{
    m_tempRenderTargetPool = std::make_unique<TempRenderTargetPool>();
    LoadPipelineDefinition(pipelineDefinitionPath);
}

void DataDrivenPipelineManager::LoadPipelineDefinition(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        // In a real engine, use a proper logging system
        std::cerr << "Error: Failed to open pipeline definition file: " << path << std::endl;
        return;
    }

    nlohmann::json j;
    try
    {
        file >> j;
    }
    catch (nlohmann::json::parse_error& e)
    {
        std::cerr << "Error: Failed to parse pipeline JSON: " << e.what() << std::endl;
        return;
    }

    if (j.contains("render_targets"))
    {
        CreateRenderTargets(j["render_targets"]);
    }

    if (j.contains("passes"))
    {
        CreatePasses(j["passes"]);
    }
}

void DataDrivenPipelineManager::CreateRenderTargets(const nlohmann::json& rtDefs)
{
    auto graphics = g_Engine->GetGraphics();
    UINT width = graphics->GetRenderWidth();
    UINT height = graphics->GetRenderHeight();

    for (const auto& rtDef : rtDefs)
    {
        std::string name = rtDef.at("name").get<std::string>();
        std::string formatStr = rtDef.at("format").get<std::string>();
        DXGI_FORMAT format = StringToDXGIFormat(formatStr);

        if (format == DXGI_FORMAT_UNKNOWN)
        {
            std::cerr << "Warning: Unknown render target format '" << formatStr << "' for target '" << name << "'" << std::endl;
            continue;
        }

        if (format == DXGI_FORMAT_D32_FLOAT) // Or other depth formats
        {
            m_renderTargets[name] = std::make_shared<DepthStencilTarget>(width, height, format);
        }
        else
        {
            m_renderTargets[name] = std::make_shared<RenderTarget>(width, height, format);
        }
    }
}

void DataDrivenPipelineManager::CreatePasses(const nlohmann::json& passDefs)
{
    for (const auto& passDef : passDefs)
    {
        std::string type = passDef.at("type").get<std::string>();
        std::shared_ptr<IRenderPass> passInstance = PassFactory::CreatePass(type);

        if (!passInstance)
        {
            std::cerr << "Warning: Failed to create pass of type '" << type << "'. Skipping." << std::endl;
            continue;
        }

        PipelinePass p;
        p.pass = passInstance;
        p.name = passDef.at("name").get<std::string>();

        if (passDef.contains("enabled_by"))
        {
            p.enabled_by_key = passDef.at("enabled_by").get<std::string>();
        }

        if (passDef.contains("inputs"))
        {
            for (const auto& input : passDef["inputs"])
            {
                p.inputs.push_back(input.get<std::string>());
            }
        }

        if (passDef.contains("outputs"))
        {
            for (const auto& output : passDef["outputs"])
            {
                p.outputs.push_back(output.get<std::string>());
            }
        }

        m_passes.push_back(p);
        m_passMap[p.name] = p.pass;
    }
}

void DataDrivenPipelineManager::Execute()
{
    auto graphics = g_Engine->GetGraphics();
    auto cmdList = graphics->GetCommandList();

    RenderContext context;
    context.CommandList = cmdList;
    context.PipelineStateManager = g_Engine->GetPSOCache();
    context.DescriptorHeap = g_Engine->GetDescriptorHeap();
    context.RenderTargetPool = m_tempRenderTargetPool.get();
    context.RenderTargetStore = &m_renderTargets;

    for (const auto& p : m_passes)
    {
        // Check if the pass is enabled
        if (!p.enabled_by_key.empty())
        {
            if (!m_settings->GetBool(p.enabled_by_key, false))
            {
                continue; // Skip this pass if the setting is false
            }
        }

        // Here you would set up the context with the correct input/output targets
        // based on p.inputs and p.outputs. This is a simplified version.
        // For a full implementation, you'd need a more robust way to bind these.
        if (!p.inputs.empty())
        {
            context.SetSourceRT(GetRenderTargetForPreview(p.inputs[0]));
        }
        if (!p.outputs.empty())
        {
            if (p.outputs[0] == "BACKBUFFER")
            {
                context.SetDestRT(nullptr); // Special case for backbuffer
            }
            else
            {
                context.SetDestRT(GetRenderTargetForPreview(p.outputs[0]));
            }
        }
        
        p.pass->Execute(context);
    }
}

std::shared_ptr<ITargetBase> DataDrivenPipelineManager::GetRenderTargetForPreview(const std::string& name) const
{
    auto it = m_renderTargets.find(name);
    if (it != m_renderTargets.end())
    {
        return it->second;
    }
    return nullptr;
}