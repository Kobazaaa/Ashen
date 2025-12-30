// -- Ashen Includes --
#include "Renderer.h"
#include "Image.h"
#include "Timer.h"
#include "Types.h"
#include "ConsoleTextSettings.h"

// -- Math Includes --
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/norm.hpp"

// -- Standard Library --
#include <iostream>

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Renderer::Renderer(Window* pWindow)
	: m_pWindow(pWindow)
	, m_pContext(std::make_unique<VulkanContext>(pWindow))
{
    constexpr float pi = std::numbers::pi_v<float>;

    // -- Rayleigh --
    constexpr float iorAir              { 1.0003f };                                // Index of Refraction Air
    constexpr float molecularDensity    { 2.545e25f };                              // Molecular Density of Air at sea level
    constexpr glm::vec3 wavelength      { 6.5e-7f, 5.1e-7f, 4.75e-7f };             // Wavelengths for RGB in order in m
    constexpr float K = 2 * pi * pi * (iorAir * iorAir - 1) * (iorAir * iorAir - 1) // constant for standard atmosphere
						/ 
					(3 * molecularDensity);
    m_BetaRayleigh = 4 * pi * K
						/ 
					glm::pow(wavelength, glm::vec3{4.f});

    // -- Mie --
    m_BetaMie = glm::vec3(2e-6f, 2e-6f, 2e-6f);

    // -- Ozone --
    constexpr glm::vec3 ozoneAbsorptionCrossSection = glm::vec3(3.1e-25, 1.9e-25, 4.5e-26);
    m_BetaOzone = ozoneAbsorptionCrossSection * molecularDensity;

    // -- Camera --
    m_pCamera = std::make_unique<Camera>(pWindow);
    m_pCamera->Position.y = m_RenderPlanetRadius + m_RenderAtmosphereThickness * 0.001f;
    m_pCamera->Speed /= 10;
    m_pCamera->Rotation = { -25.f, 0.f, 0.f };

    // -- Lights --
    m_vLightDirections.resize(3);

    float angle = 2.5f;
    m_vLightDirections[0] = glm::vec3(0.f, sin(glm::radians(angle)), cos(glm::radians(angle)));
    angle = 20.f;
    m_vLightDirections[1] = glm::vec3(0.f, sin(glm::radians(angle)), cos(glm::radians(angle)));
    angle = 40.f;
    m_vLightDirections[2] = glm::vec3(0.f, sin(glm::radians(angle)), cos(glm::radians(angle)));

    m_LightDirection = m_vLightDirections[m_LightIndex];

    // -- Render --
	CreateSyncObjects();

    m_pMeshFloor    = CreateDome(m_RenderPlanetRadius, 250, 250);
    m_pMeshSky      = CreateDome(m_RenderPlanetRadius + m_RenderAtmosphereThickness, 250, 250);

    const auto count = m_pContext->GetSwapchainImageCount();
    m_vUBOGround_VS = { *m_pContext, count };

    m_vUBOSky_VS    = { *m_pContext, count };
    m_vUBOSky_FS    = { *m_pContext, count };

    CreateSamplers();
    CreateDepthResources(m_pContext->GetSwapchainExtent());
    CreateRenderTargets(m_pContext->GetSwapchainExtent());
    CreateDescriptorSets();

    CreatePipelines(m_UseHDR ? m_vRenderTargets.front().GetFormat() : m_pContext->GetSwapchainFormat());
    CreateCommandBuffers();
}
ashen::Renderer::~Renderer()
{
    VkDevice device = m_pContext->GetDevice();
    vkDeviceWaitIdle(device);

    vkDestroySampler(m_pContext->GetDevice(), m_PostProcessSampler, nullptr);

	for (const auto& sem : m_vImageAvailableSemaphores) vkDestroySemaphore(device, sem, nullptr);
	for (const auto& sem : m_vRenderFinishedSemaphores) vkDestroySemaphore(device, sem, nullptr);
	for (const auto& fence : m_vInFlightFences) vkDestroyFence(device, fence, nullptr);
}


//--------------------------------------------------
//    Functionality
//--------------------------------------------------
void ashen::Renderer::Update()
{
    HandleInput();

    float scaledHeight = m_PlanetRadius + (glm::length(m_pCamera->Position) - m_RenderPlanetRadius) / (m_RenderAtmosphereThickness) * m_AtmosphereThickness;
    glm::vec3 scaledPos = normalize(m_pCamera->Position) * scaledHeight;

    SkyVS skyVs
    {
        .cameraPos = scaledPos,
        .cameraHeight = scaledHeight,

        .lightDir = m_LightDirection,
        .sampleCount = static_cast<float>(m_SampleCount),

        .betaR = m_BetaRayleigh,
        .atmosphereThickness = m_AtmosphereThickness,

        .betaM = m_BetaMie,
        .planetRadius = m_PlanetRadius,

        .betaO = m_UseOzone ? m_BetaOzone : glm::vec3(0.f),
        .rayleighScaleHeight = m_RayleighScaleDepth,

        .mieScaleHeigh = m_MieScaleDepth,
        .sunIntensity = m_ESun,

        .renderRadius = m_RenderPlanetRadius,
        .renderThickness = m_RenderAtmosphereThickness
    };
    SkyFS skyFs
    {
        .lightDir = m_LightDirection,
        .g = m_g,
        .g2 = m_g * m_g,
        .phaseType = m_PhaseFunctionIndex
    };
    m_vUBOSky_VS[m_CurrentFrame].MapData(&skyVs, sizeof(SkyVS));
    m_vUBOSky_FS[m_CurrentFrame].MapData(&skyFs, sizeof(SkyFS));



    GroundVS groundVs
    {
        .lightDir = m_LightDirection,
    };
    m_vUBOGround_VS[m_CurrentFrame].MapData(&groundVs, sizeof(GroundVS));
}
void ashen::Renderer::Render()
{
    VkDevice device = m_pContext->GetDevice();
    VkSwapchainKHR swapchain = m_pContext->GetSwapchain();

    vkWaitForFences(device, 1, &m_vInFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, m_vImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        OnResize();
    	return;
    }
    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to acquire Swap Chain Image");

    vkResetFences(device, 1, &m_vInFlightFences[m_CurrentFrame]);

	RecordCommandBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_vImageAvailableSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_vCommandBuffers[m_CurrentFrame];

    VkSemaphore signalSemaphores[] = { m_vRenderFinishedSemaphores[m_CurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(m_pContext->GetQueue(vkb::QueueType::graphics), 1, &submitInfo, m_vInFlightFences[m_CurrentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(m_pContext->GetQueue(vkb::QueueType::present), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_pWindow->IsOutdated())
    {
        m_pWindow->ResetOutdated();
        OnResize();
    }
    else if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to present Swap Chain Image!");

    m_CurrentFrame = (m_CurrentFrame + 1) % m_vInFlightFences.size();
}
void ashen::Renderer::HandleInput()
{
    // -- Variables --
	float deltaT = Timer::GetDeltaSeconds();
    if (m_pWindow->IsKeyDown(GLFW_KEY_RIGHT_SHIFT)) deltaT *= 3.f;
    const float exposureChange = 0.5f * deltaT;
    const float gChange = 0.05f * deltaT;
    const float eSunChange = 1.f * deltaT;
    const float sunDirChange = 0.1f * deltaT;

    // -- Camera --
    m_pCamera->Update();

    // -- Samples --
    static bool kPAddPrev = false;
    static bool kPSubtractPrev = false;
    const bool kpAdd = m_pWindow->IsKeyDown(GLFW_KEY_KP_ADD);
    const bool kpSub = m_pWindow->IsKeyDown(GLFW_KEY_KP_SUBTRACT);
    if (kpAdd && !kPAddPrev) ++m_SampleCount;
    if (kpSub && !kPSubtractPrev) --m_SampleCount;
    kPAddPrev = kpAdd;
    kPSubtractPrev = kpSub;

    // -- HDR --
    static bool tabPrev = false;
    const bool tabCurr = m_pWindow->IsKeyDown(GLFW_KEY_TAB);
    if (tabCurr && !tabPrev)
    {
        m_UseHDR = !m_UseHDR;
        CreatePipelines(m_UseHDR ? m_vRenderTargets.front().GetFormat() : m_pContext->GetSwapchainFormat());
    }
    tabPrev = tabCurr;

    // -- Scattering --
    if (m_pWindow->IsKeyDown(GLFW_KEY_3))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_g = std::max(-1.0f, m_g - gChange);
        else m_g = std::min(m_g + gChange, 1.0f);
    }
    else if (m_pWindow->IsKeyDown(GLFW_KEY_4))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_ESun = std::max(0.0f, m_ESun - eSunChange);
        else m_ESun += eSunChange;
    }

    // -- Exposure --
    else if (m_pWindow->IsKeyDown(GLFW_KEY_8))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_Exposure -= exposureChange;
        else m_Exposure += exposureChange;
    }

    // -- Light --
    static float azimuth = atan2(m_LightDirection.z, m_LightDirection.x);
    static float elevation = acos(glm::clamp(m_LightDirection.y, -1.0f, 1.0f));

    if (m_pWindow->IsKeyDown(GLFW_KEY_UP))    elevation -= sunDirChange;
    if (m_pWindow->IsKeyDown(GLFW_KEY_DOWN))  elevation += sunDirChange;
    if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT))  azimuth += sunDirChange;
    if (m_pWindow->IsKeyDown(GLFW_KEY_RIGHT)) azimuth -= sunDirChange;

    static bool nPrev = false;
    const bool nCurr = m_pWindow->IsKeyDown(GLFW_KEY_9);
    if (nCurr && !nPrev)
    {
        m_LightIndex = (m_LightIndex + 1) % static_cast<uint32_t>(m_vLightDirections.size());
        m_LightDirection = m_vLightDirections[m_LightIndex];
        azimuth = atan2(m_LightDirection.z, m_LightDirection.x);
        elevation = acos(glm::clamp(m_LightDirection.y, -1.0f, 1.0f));
    }
    nPrev = nCurr;

    m_LightDirection = glm::normalize(glm::vec3(
        sin(elevation) * cos(azimuth),
        cos(elevation),
        sin(elevation) * sin(azimuth))
    );


    // -- Ozone --
    static bool oPrev = false;
    const bool oCurr = m_pWindow->IsKeyDown(GLFW_KEY_O);
    if (oCurr && !oPrev)
        m_UseOzone = !m_UseOzone;
    oPrev = oCurr;

    // -- Phase Function --
    static bool fPrev = false;
    const bool fCurr = m_pWindow->IsKeyDown(GLFW_KEY_F);
    if (fCurr && !fPrev)
        m_PhaseFunctionIndex = (m_PhaseFunctionIndex + 1) % m_PhaseFunctionCount;
    fPrev = fCurr;


    PrintStats();
}
void ashen::Renderer::PrintStats()
{
    // -- FPS calculation over 1 second --
    static float elapsedTime = 0.f;
    static int frames = 0;
    static float fps = 0.f;

    elapsedTime += Timer::GetDeltaSeconds();
    frames++;

    if (elapsedTime >= 1.0f)
    {
        fps = static_cast<float>(frames) / elapsedTime;
        frames = 0;
        elapsedTime = 0.f;
    }
    else return;

    // -- Move cursor up to overwrite previous stats --
    static bool first = true;
    if (!first)
        std::cout << "\033[11A";
	first = false;

    // -- Print stats with keybind hints --
    std::cout << "--- STATS OVERVIEW ---\n";
    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key + / Key -]" << RESET_TXT
				<< "\t\t\tSamples: " << m_SampleCount << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 3 / Shift + 3]" << RESET_TXT
				<< "\t\tg: " << m_g << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 4 / Shift + 4]" << RESET_TXT
				<< "\t\tESun: " << m_ESun << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Tab]" << RESET_TXT
				<< "\t\t\t\tHDR: " << (m_UseHDR ? BRIGHT_GREEN_TX : BRIGHT_RED_TXT) << (m_UseHDR ? "True" : "False") << RESET_TXT << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 8 / Shift + 8]" << RESET_TXT
        << "\t\tExposure: " << m_Exposure << "\n";

	std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[O]" << RESET_TXT
				<< "\t\t\t\tOzone: " << (m_UseOzone ? BRIGHT_GREEN_TX : BRIGHT_RED_TXT) << (m_UseOzone ? "True" : "False") << RESET_TXT << "\n";

    std::string phaseFunctionName = "Unknown";
    if (m_PhaseFunctionIndex == 0) phaseFunctionName = "Henyey-Greenstein";
    if (m_PhaseFunctionIndex == 1) phaseFunctionName = "Cornette-Shanks";
    if (m_PhaseFunctionIndex == 2) phaseFunctionName = "Double Henyey-Greenstein";
    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[F]" << RESET_TXT
        << "\t\t\t\tPhase Functions: " << DARK_CYAN_TXT << phaseFunctionName << RESET_TXT << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 9 / Shift + 9]" << RESET_TXT
        << "\t\tLight Preset: " << m_LightIndex << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[X]" << RESET_TXT
				<< "\t\t\t\tFPS: " << DARK_YELLOW_TXT << fps  << RESET_TXT << "\n";

    std::cout << "--- STATS OVERVIEW ---\n";
    std::cout << std::flush;
}


//--------------------------------------------------
//    Helpers
//--------------------------------------------------
// -- Meshes --
std::unique_ptr<ashen::Mesh> ashen::Renderer::CreateDome(float radius, int segmentsLat, int segmentsLon) const
{
    // -- Data --
    constexpr float VERTEX_COLOR_CHANNEL = 0.5f;
    constexpr glm::vec3 VERTEX_COLOR = { VERTEX_COLOR_CHANNEL , VERTEX_COLOR_CHANNEL , VERTEX_COLOR_CHANNEL };

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // -- Vertices --
    for (int lat{}; lat <= segmentsLat; ++lat) 
    {
        const float theta = glm::half_pi<float>() * static_cast<float>(lat) / static_cast<float>(segmentsLat);
        const float sinTheta = sin(theta);
        const float cosTheta = cos(theta);

        for (int lon{}; lon <= segmentsLon; ++lon)
        {
            const float phi = glm::two_pi<float>() * static_cast<float>(lon) / static_cast<float>(segmentsLon);
            const float sinPhi = sin(phi);
            const float cosPhi = cos(phi);

            const glm::vec3 pos = radius * glm::vec3(cosPhi * sinTheta, cosTheta, sinPhi * sinTheta);
            vertices.push_back(
                {
                	.pos = pos,
                	.color = VERTEX_COLOR
                });
        }
    }

    // -- Indices --
    for (int lat{}; lat < segmentsLat; ++lat) 
    {
        for (int lon{}; lon < segmentsLon; ++lon) 
        {
            const int current = lat * (segmentsLon + 1) + lon;
            const int next = current + segmentsLon + 1;

            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            indices.push_back(next);
            indices.push_back(current + 1);
            indices.push_back(next + 1);
        }
    }

    return std::make_unique<Mesh>(*m_pContext,
        vertices,
        indices
    );
}

// -- Creation --
void ashen::Renderer::CreateSamplers()
{
    VkSamplerCreateInfo smaplerInfo{};
    smaplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    smaplerInfo.magFilter = VK_FILTER_NEAREST;
    smaplerInfo.minFilter = VK_FILTER_NEAREST;
    smaplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    smaplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    smaplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    smaplerInfo.anisotropyEnable = VK_FALSE;
    smaplerInfo.maxAnisotropy = 0;
    smaplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    smaplerInfo.unnormalizedCoordinates = VK_FALSE;
    smaplerInfo.compareEnable = VK_FALSE;
    smaplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    smaplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    smaplerInfo.mipLodBias = 0.0f;
    smaplerInfo.minLod = 0.0f;
    smaplerInfo.maxLod = VK_LOD_CLAMP_NONE;

    if (vkCreateSampler(m_pContext->GetDevice(), &smaplerInfo, nullptr, &m_PostProcessSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Texture Sampler!");
}
void ashen::Renderer::CreatePipelines(VkFormat renderFormat)
{
    vkDeviceWaitIdle(m_pContext->GetDevice());
    m_GroundFromAtmosphere.Destroy();
    m_SkyFromAtmosphere.Destroy();
    m_PostProcess.Destroy();

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    VkFormat format = renderFormat;
    VkFormat swapchainFormat = m_pContext->GetSwapchainFormat();
    pipelineRenderingInfo.pColorAttachmentFormats = &format;
    pipelineRenderingInfo.depthAttachmentFormat = m_vDepthImages.front().GetFormat();

    auto attr = Vertex::GetAttributeDescriptions();
    auto bind = Vertex::GetBindingDescription();

    PipelineBuilder pipelineBuilder{ *m_pContext };
    pipelineBuilder
        .SetVertexAttributeDesc(attr)
        .SetVertexBindingDesc(bind)

        .AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)

        .SetDepthTest(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS)

        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetPolygonMode(VK_POLYGON_MODE_FILL)

        .SetupDynamicRendering(pipelineRenderingInfo);

    std::string prefix = "shaders/";
    std::string vert = ".vert.spv";
    std::string frag = ".frag.spv";

    pipelineBuilder
        .AddPushConstantRange()
	        .SetSize(sizeof(CameraMatricesPC))
	        .SetOffset(0)
	        .SetStageFlags(VK_SHADER_STAGE_VERTEX_BIT)
	        .EndRange()
        .AddDescriptorSet(m_vDescriptorSetsGround.front())
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetVertexShader(prefix + "GroundFromAtmosphere" + vert)
        .SetFragmentShader(prefix + "GroundFromAtmosphere" + frag)
        .Build(m_GroundFromAtmosphere);

    pipelineBuilder
        .AddPushConstantRange()
	        .SetSize(sizeof(CameraMatricesPC))
	        .SetOffset(0)
	        .SetStageFlags(VK_SHADER_STAGE_VERTEX_BIT)
	        .EndRange()
        .AddDescriptorSet(m_vDescriptorSetsSky.front())
        .SetCullMode(VK_CULL_MODE_FRONT_BIT)
        .SetVertexShader(prefix + "SkyFromAtmosphere" + vert)
        .SetFragmentShader(prefix + "SkyFromAtmosphere" + frag)
        .SetDepthTest(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS)
        .EnableColorBlend(0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD)
        .EnableAlphaBlend(0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD)
        .Build(m_SkyFromAtmosphere);


    // post process
    pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;
    pipelineBuilder = { *m_pContext };
    pipelineBuilder
        .AddPushConstantRange()
            .SetSize(sizeof(Exposure))
            .SetOffset(0)
            .SetStageFlags(VK_SHADER_STAGE_FRAGMENT_BIT)
            .EndRange()
        .AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT)
        .AddDynamicState(VK_DYNAMIC_STATE_SCISSOR)
        .SetCullMode(VK_CULL_MODE_BACK_BIT)
        .SetFrontFace(VK_FRONT_FACE_CLOCKWISE)
        .SetPrimitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetPolygonMode(VK_POLYGON_MODE_FILL)
        .SetupDynamicRendering(pipelineRenderingInfo)
        .AddDescriptorSet(m_vDescriptorSetsPostProcess.front())
        .SetVertexShader(prefix + "FullscreenTri" + vert)
        .SetFragmentShader(prefix + "PostProcess" + frag)
        .SetDepthTest(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER)
        .Build(m_PostProcess);

}
void ashen::Renderer::CreateDescriptorSets()
{
    DescriptorPoolBuilder builder{ *m_pContext };
    auto count = m_pContext->GetSwapchainImageCount();
    builder
        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count * 3 * 2)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count)
        .SetMaxSets(count * 4)
        .SetFlags(0)
        .Build(m_DescriptorPool);

    m_vDescriptorSetsSky.resize(count);
    m_vDescriptorSetsGround.resize(count);
    m_vDescriptorSetsPostProcess.resize(count);
    for (uint32_t i{}; i < count; ++i)
    {
        DescriptorSetAllocator allocator{ *m_pContext };
        DescriptorSetWriter writer{ *m_pContext };

        allocator
            .NewLayoutBinding()
	            .SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	            .SetCount(1)
	            .SetShaderStages(VK_SHADER_STAGE_VERTEX_BIT)
	            .EndLayoutBinding()
            .NewLayoutBinding()
	            .SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	            .SetCount(1)
	            .SetShaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
	            .EndLayoutBinding()
            .Allocate(m_DescriptorPool, m_vDescriptorSetsSky[i]);

        allocator
            .NewLayoutBinding()
	            .SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	            .SetCount(1)
	            .SetShaderStages(VK_SHADER_STAGE_VERTEX_BIT)
	            .EndLayoutBinding()
            .Allocate(m_DescriptorPool, m_vDescriptorSetsGround[i]);

        allocator
            .NewLayoutBinding()
                .SetType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .SetCount(1)
                .SetShaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                .EndLayoutBinding()
            .Allocate(m_DescriptorPool, m_vDescriptorSetsPostProcess[i]);

        writer
            .AddBufferInfo(m_vUBOSky_VS[i], 0, sizeof(SkyVS))
            .WriteBuffers(m_vDescriptorSetsSky[i], 0)
            .Execute();
        writer
            .AddBufferInfo(m_vUBOSky_FS[i], 0, sizeof(SkyFS))
            .WriteBuffers(m_vDescriptorSetsSky[i], 1)
            .Execute();

        writer
            .AddBufferInfo(m_vUBOGround_VS[i], 0, sizeof(GroundVS))
            .WriteBuffers(m_vDescriptorSetsGround[i], 0)
            .Execute();

        writer
            .AddImageInfo((m_vRenderTargets)[i].GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_PostProcessSampler)
            .WriteImages(m_vDescriptorSetsPostProcess[i], 0)
            .Execute();
    }
}
void ashen::Renderer::CreateDepthResources(VkExtent2D extent)
{
    m_vDepthImages.clear();
    m_vDepthImages.resize(m_pContext->GetSwapchainImageCount());
    for (Image& image : m_vDepthImages)
    {
        const auto format = Image::FindSupportedFormat(m_pContext->GetPhysicalDevice(),
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

        ImageBuilder imageBuilder{ *m_pContext };
        imageBuilder
            .SetWidth(extent.width)
            .SetHeight(extent.height)
            .SetTiling(VK_IMAGE_TILING_OPTIMAL)
            .SetAspectFlags(VK_IMAGE_ASPECT_DEPTH_BIT)
            .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
            .SetFormat(format)
            .SetUsageFlags(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            .Build(image);
    }
}
void ashen::Renderer::CreateRenderTargets(VkExtent2D extent)
{
    m_vRenderTargets.clear();
    m_vRenderTargets.resize(m_pContext->GetSwapchainImageCount());
    for (Image& image : m_vRenderTargets)
    {
        ImageBuilder imageBuilder{ *m_pContext };
        imageBuilder
            .SetWidth(extent.width)
            .SetHeight(extent.height)
            .SetTiling(VK_IMAGE_TILING_OPTIMAL)
            .SetFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
            .SetAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT)
            .SetViewType(VK_IMAGE_VIEW_TYPE_2D)
            .SetUsageFlags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
            .Build(image);
    }
}
void ashen::Renderer::CreateCommandBuffers()
{
    VkDevice device = m_pContext->GetDevice();

    // -- Command Buffers --
    m_vCommandBuffers.resize(m_pContext->GetSwapchainImageCount());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_pContext->GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_vCommandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, m_vCommandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers");
}
void ashen::Renderer::CreateSyncObjects()
{
    // -- Resize Vectors --
    VkDevice device = m_pContext->GetDevice();
    size_t maxFramesInFlight = 2;
    m_vImageAvailableSemaphores.resize(maxFramesInFlight);
    m_vRenderFinishedSemaphores.resize(maxFramesInFlight);
    m_vInFlightFences.resize(maxFramesInFlight);

    // -- Semaphore Info --
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // -- Fence Info --
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Creation --
    for (size_t i = 0; i < maxFramesInFlight; i++) 
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_vImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_vRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &m_vInFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create sync objects");
        }
    }
}

// -- Frame --
void ashen::Renderer::SetupFrame(uint32_t imageIndex) const
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer!");
    }

    VkImageMemoryBarrier presentBarrier{};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.srcAccessMask = VK_ACCESS_NONE;
    presentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.image = m_pContext->GetSwapchainImages()[imageIndex];
    presentBarrier.subresourceRange =
    {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0, .levelCount = 1,
        .baseArrayLayer = 0, .layerCount = 1
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_NONE,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &presentBarrier
    );
}
void ashen::Renderer::SetRenderTarget(VkImageView view, VkImageLayout layout)
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];
    Image& depthImage = m_vDepthImages[m_CurrentFrame];

    // Dynamic rendering info
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { {0.f, 0.f, 0.f, 1.0f} };

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImage.GetView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = { 0, 0 };
    renderingInfo.renderArea.extent = m_pContext->GetSwapchainExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);
}
void ashen::Renderer::EndRenderTarget() const
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];
    vkCmdEndRendering(cmd);
}
void ashen::Renderer::RenderFrame(uint32_t imageIndex)
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];
    Image& renderImage = m_vRenderTargets[m_CurrentFrame];

    CameraMatricesPC camMatrices
	{
    	.view = m_pCamera->GetViewMatrix(),
    	.proj = m_pCamera->GetProjectionMatrix()
    };

    // Transition to be renderable
    if (m_UseHDR)
    {
	    renderImage.TransitionLayout(cmd,
	        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	        VK_ACCESS_2_NONE, VK_PIPELINE_STAGE_2_NONE,
	        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    }

	SetRenderTarget(m_UseHDR ? renderImage.GetView() : m_pContext->GetSwapchainImageViews()[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    {
        // -- Ground Objects --
        Pipeline* pGroundShader = &m_GroundFromAtmosphere;
        pGroundShader->Bind(cmd);
        m_pMeshFloor->Bind(cmd);
        vkCmdPushConstants(cmd, pGroundShader->GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(CameraMatricesPC), &camMatrices);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pGroundShader->GetLayoutHandle(), 0, 1,
            &m_vDescriptorSetsGround[m_CurrentFrame].GetHandle(), 0, nullptr);

        m_pMeshFloor->Draw(cmd);

        // -- Sky Objects --
        Pipeline* pSkyShader = &m_SkyFromAtmosphere;
        pSkyShader->Bind(cmd);
        m_pMeshSky->Bind(cmd);
        vkCmdPushConstants(cmd, pSkyShader->GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(CameraMatricesPC), &camMatrices);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pSkyShader->GetLayoutHandle(), 0, 1,
            &m_vDescriptorSetsSky[m_CurrentFrame].GetHandle(), 0, nullptr);

        m_pMeshSky->Draw(cmd);
    }
    EndRenderTarget();

    if (!m_UseHDR)
        return;

    // Transition to be readable
    renderImage.TransitionLayout(cmd,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    SetRenderTarget(m_pContext->GetSwapchainImageViews()[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    {
        Exposure exposure
        {
            .exposure = m_Exposure,
        };
        m_PostProcess.Bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PostProcess.GetLayoutHandle(), 0, 1, 
            &m_vDescriptorSetsPostProcess[m_CurrentFrame].GetHandle(), 0, nullptr);
        vkCmdPushConstants(cmd, m_PostProcess.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Exposure), &exposure);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
    EndRenderTarget();
}
void ashen::Renderer::EndFrame(uint32_t imageIndex) const
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];

    VkImageMemoryBarrier presentBarrier{};
	presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.dstAccessMask = 0;
    presentBarrier.image = m_pContext->GetSwapchainImages()[imageIndex];
    presentBarrier.subresourceRange =
    {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0, .levelCount = 1,
        .baseArrayLayer = 0, .layerCount = 1
    };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &presentBarrier
    );


    if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void ashen::Renderer::OnResize()
{
    auto size = m_pWindow->GetFramebufferSize();
    while (size.x == 0 || size.y == 0)
    {
        size = m_pWindow->GetFramebufferSize();
        m_pWindow->PollEvents();
    }

    vkDeviceWaitIdle(m_pContext->GetDevice());
    m_pContext->RebuildSwapchain(size);
    CreateDepthResources(m_pContext->GetSwapchainExtent());
    CreateRenderTargets(m_pContext->GetSwapchainExtent());

    auto count = m_pContext->GetSwapchainImageCount();
    for (uint32_t i{}; i < count; ++i)
    {
        DescriptorSetWriter writer{ *m_pContext };
        writer
            .AddImageInfo((m_vRenderTargets)[i].GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_PostProcessSampler)
            .WriteImages(m_vDescriptorSetsPostProcess[i], 0)
            .Execute();
    }


    for (const auto& sem : m_vImageAvailableSemaphores) vkDestroySemaphore(m_pContext->GetDevice(), sem, nullptr);
    for (const auto& sem : m_vRenderFinishedSemaphores) vkDestroySemaphore(m_pContext->GetDevice(), sem, nullptr);
    for (const auto& fence : m_vInFlightFences) vkDestroyFence(m_pContext->GetDevice(), fence, nullptr);
    CreateSyncObjects();

    m_pCamera->AspectRatio = m_pWindow->GetAspectRatio();
}

void ashen::Renderer::RecordCommandBuffer(uint32_t imageIndex)
{
    SetupFrame(imageIndex);
    RenderFrame(imageIndex);
    EndFrame(imageIndex);
}
