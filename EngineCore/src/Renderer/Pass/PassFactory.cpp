#include "PassFactory.h"

// Include all concrete pass headers
#include "ComputeProcess/RainParticleSystem.h"
#include "PostProcess/Manager/PostProcessManager.h"
#include "PostProcess/Pass/FXAAPass.h"
#include "PostProcess/Pass/TAAPass.h"
#include "PostProcess/Pass/UnjitterPass.h"
#include "PostProcess/Pass/BloomPass.h"
#include "ShadowProcess/Pass/CascadedShadowMapPass.h"
#include "ShadowProcess/Pass/SimpleShadowMapPass.h"
#include "ShadowProcess/Pass/RayTracedShadowPass.h"
#include "RenderProcess/Pass/SkyboxPass.h"
#include "RenderProcess/Pass/DebugPass.h"
#include "RenderProcess/Pass/LightingPass.h"
#include "RenderProcess/Pass/SSAOPass.h"
#include "RenderProcess/Pass/RTAODenoisePass.h"
#include "RenderProcess/Pass/RTAOPass.h"
#include "RenderProcess/Pass/RTGIPass.h"
#include "RenderProcess/Pass/RTGIDenoisePass.h"
#include "RenderProcess/Pass/RTReflectionPass.h"
#include "RenderProcess/Pass/SSRPass.h"
#include "RenderProcess/Pass/SSRCompositePass.h"
#include "RenderProcess/Pass/GeometryPass.h"


std::shared_ptr<IRenderPass> PassFactory::CreatePass(const std::string& passName)
{
    if (passName == "SimpleShadowMap") return std::make_shared<SimpleShadowMapPass>();
    if (passName == "RayTracedShadow") return std::make_shared<RayTracedShadowPass>();
    if (passName == "CascadedShadowMap") return std::make_shared<CascadedShadowMapPass>();
    
    if (passName == "Geometry") return std::make_shared<GeometryPass>();
    if (passName == "Skybox") return std::make_shared<SkyboxPass>();
    if (passName == "Lighting") return std::make_shared<LightingPass>();
    if (passName == "Debug") return std::make_shared<DebugPass>();

    if (passName == "SSAO") return std::make_shared<SSAOPass>();
    if (passName == "SSR") return std::make_shared<SSRPass>();
    if (passName == "SSRComposite") return std::make_shared<SSRCompositePass>();

    if (passName == "RTAO") return std::make_shared<RTAOPass>();
    if (passName == "RTAODenoise") return std::make_shared<RTAODenoisePass>();
    if (passName == "RTGI") return std::make_shared<RTGIPass>();
    if (passName == "RTGIDenoise") return std::make_shared<RTGIDenoisePass>();
    if (passName == "RTReflection") return std::make_shared<RTReflectionPass>();

    if (passName == "PostProcessManager") return std::make_shared<PostProcessManager>();
    if (passName == "FXAA") return std::make_shared<FXAAPass>();
    if (passName == "TAA") return std::make_shared<TAAPass>();
    if (passName == "Unjitter") return std::make_shared<UnjitterPass>();
    if (passName == "Bloom") return std::make_shared<BloomPass>();

    if (passName == "RainParticle") return std::make_shared<RainParticleSystem>();

    // If the pass name is not recognized, return nullptr.
    // Consider logging an error here.
    return nullptr;
}