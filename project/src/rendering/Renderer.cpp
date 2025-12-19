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
    m_pCamera = std::make_unique<Camera>(pWindow);
    m_pCamera->Position.y = m_InnerRadius + (m_OuterRadius - m_InnerRadius) * 0.1f;
    m_pCamera->Speed /= 10;
	CreateSyncObjects();

    m_pMeshFloor    = CreateDome(m_InnerRadius, 250, 250);
    m_pMeshSky      = CreateDome(m_OuterRadius, 250, 250);

    const auto count = m_pContext->GetSwapchainImageCount();
    m_vUBOSpace_VS  = { *m_pContext, count };
    m_vUBOSpace_FS  = { *m_pContext, count };

    m_vUBOGround_VS = { *m_pContext, count };
    m_vUBOGround_FS = { *m_pContext, count };

    m_vUBOSky_VS    = { *m_pContext, count };
    m_vUBOSky_FS    = { *m_pContext, count };

    CreateDepthResources(m_pContext->GetSwapchainExtent());
    CreateDescriptorSets();

    CreatePipelines();
	CreateCommandBuffers();
}
ashen::Renderer::~Renderer()
{
    VkDevice device = m_pContext->GetDevice();
    vkDeviceWaitIdle(device);

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

    SkyVS skyVs
    {
        .cameraPos = m_pCamera->Position,
        .cameraHeight = glm::length(m_pCamera->Position),

        .lightDir = m_LightDirection,
        .cameraHeight2 = glm::dot(m_pCamera->Position, m_pCamera->Position),

        .invWaveLength = 1.f / m_Wavelength4,
        .sampleCount = static_cast<float>(m_SampleCount),

        .outerRadius = m_OuterRadius,
        .outerRadius2 = m_OuterRadius * m_OuterRadius,
        .innerRadius = m_InnerRadius,
        .innerRadius2 = m_InnerRadius * m_InnerRadius,

        .scale = m_Scale,
        .scaleDepth = m_RayleighScaleDepth,
        .scaleOverScaleDepth = m_Scale / m_RayleighScaleDepth,
        .invScaleDepth = 1.f / m_RayleighScaleDepth,

        .krESun = m_Kr * m_ESun,
        .kmESun = m_Km * m_ESun,
        .kr4PI = m_Kr4PI,
        .km4PI = m_Km4PI,
    };
    SkyFS skyFs
    {
        .lightDir = m_LightDirection,
        .g = m_g,
        .g2 = m_g * m_g
    };
    m_vUBOSky_VS[m_CurrentFrame].MapData(&skyVs, sizeof(SkyVS));
    m_vUBOSky_FS[m_CurrentFrame].MapData(&skyFs, sizeof(SkyFS));



    GroundVS groundVs
    {
        .cameraPos = m_pCamera->Position,
        .cameraHeight = glm::length(m_pCamera->Position),

    	.lightDir = m_LightDirection,
        .cameraHeight2 = glm::dot(m_pCamera->Position, m_pCamera->Position),

    	.invWaveLength = 1.f / m_Wavelength4,
        .sampleCount = static_cast<float>(m_SampleCount),

        .outerRadius = m_OuterRadius,
        .outerRadius2 = m_OuterRadius * m_OuterRadius,
        .innerRadius = m_InnerRadius,
        .innerRadius2 = m_InnerRadius * m_InnerRadius,

        .scale = m_Scale,
        .scaleDepth = m_RayleighScaleDepth,
        .scaleOverScaleDepth = m_Scale / m_RayleighScaleDepth,
        .invScaleDepth = 1.f / m_RayleighScaleDepth,

        .krESun = m_Kr * m_ESun,
        .kmESun = m_Km * m_ESun,
        .kr4PI = m_Kr4PI,
        .km4PI = m_Km4PI,
    };
    GroundFS groundFs
    {
        .n = 0.f
    };
    m_vUBOGround_VS[m_CurrentFrame].MapData(&groundVs, sizeof(GroundVS));
    m_vUBOGround_FS[m_CurrentFrame].MapData(&groundFs, sizeof(GroundFS));



    SpaceVS spaceVs
    {
        .eT = Timer::GetTotalTimeSeconds()
    };
    SpaceFS spaceFx
    {
        .eT = Timer::GetTotalTimeSeconds()
    };
    m_vUBOSpace_VS[m_CurrentFrame].MapData(&spaceVs, sizeof(SpaceVS));
    m_vUBOSpace_FS[m_CurrentFrame].MapData(&spaceFx, sizeof(SpaceFS));
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

    // -- Scattering --
    if (m_pWindow->IsKeyDown(GLFW_KEY_1))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_Kr = std::max(0.0f, m_Kr - 0.0001f);
        else m_Kr += 0.0001f;
        m_Kr4PI = m_Kr * 4.0f * std::numbers::pi_v<float>;
    }
    else if (m_pWindow->IsKeyDown(GLFW_KEY_2))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_Km = std::max(0.0f, m_Km - 0.0001f);
        else m_Km += 0.0001f;
        m_Km4PI = m_Km * 4.0f * std::numbers::pi_v<float>;
    }
    else if (m_pWindow->IsKeyDown(GLFW_KEY_3))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_g = std::max(-1.0f, m_g - 0.001f);
        else m_g = std::min(m_g + 0.001f, 1.0f);
    }
    else if (m_pWindow->IsKeyDown(GLFW_KEY_4))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_ESun = std::max(0.0f, m_ESun - 0.1f);
        else m_ESun += 0.1f;
    }

    // -- Wavelengths --
    else if (m_pWindow->IsKeyDown(GLFW_KEY_5))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_Wavelength[0] = std::max(0.001f, m_Wavelength[0] - 0.001f);
        else m_Wavelength[0] += 0.001f;
        m_Wavelength4[0] = powf(m_Wavelength[0], 4.0f);
    }
    else if (m_pWindow->IsKeyDown(GLFW_KEY_6))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_Wavelength[1] = std::max(0.001f, m_Wavelength[1] - 0.001f);
        else m_Wavelength[1] += 0.001f;
        m_Wavelength4[1] = powf(m_Wavelength[1], 4.0f);
    }
    else if (m_pWindow->IsKeyDown(GLFW_KEY_7))
    {
        if (m_pWindow->IsKeyDown(GLFW_KEY_LEFT_SHIFT)) m_Wavelength[2] = std::max(0.001f, m_Wavelength[2] - 0.001f);
        else m_Wavelength[2] += 0.001f;
        m_Wavelength4[2] = powf(m_Wavelength[2], 4.0f);
    }

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


    // -- Move cursor up to overwrite previous stats --
    static bool first = true;
    if (!first)
		std::cout << "\033[9A";
    first = false;

    // -- Print stats with keybind hints --
    std::cout << "--- STATS OVERVIEW ---\n";
    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key + / Key -]" << RESET_TXT
				<< "\t\t\tSamples: " << m_SampleCount << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 1 / Shift + 1]"<< RESET_TXT
				<< "\t\tKr: " << m_Kr << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 2 / Shift + 2]" << RESET_TXT
				<< "\t\tKm : " << m_Km << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 3 / Shift + 3]" << RESET_TXT
				<< "\t\tg: " << m_g << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 4 / Shift + 4]" << RESET_TXT
				<< "\t\tESun: " << m_ESun << "\n";

    std::cout << CLEAR_LINE << BRIGHT_BLACK_TXT << "[Key 5-7 / Shift + 5-7]" << RESET_TXT
				<< "\t\tWavelengths: [" << 
						BRIGHT_RED_TXT << m_Wavelength[0] << RESET_TXT << ", " << 
						BRIGHT_GREEN_TX << m_Wavelength[1] << RESET_TXT << ", " << 
						BRIGHT_BLUE_TXT << m_Wavelength[2] << RESET_TXT << "]\n";

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
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    return std::make_unique<Mesh>(*m_pContext,
        vertices,
        indices
    );
}

// -- Creation --
void ashen::Renderer::CreatePipelines()
{
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    VkFormat swapchainFormat = m_pContext->GetSwapchainFormat();
    pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;
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

        .SetCullMode(VK_CULL_MODE_NONE)
        .SetFrontFace(VK_FRONT_FACE_CLOCKWISE)
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
        .SetVertexShader(prefix + "GroundFromSpace" + vert)
        .SetFragmentShader(prefix + "GroundFromSpace" + frag)
        .Build(m_GroundFromSpace);

    pipelineBuilder
        .AddPushConstantRange()
	        .SetSize(sizeof(CameraMatricesPC))
	        .SetOffset(0)
	        .SetStageFlags(VK_SHADER_STAGE_VERTEX_BIT)
	        .EndRange()
        .AddDescriptorSet(m_vDescriptorSetsGround.front())
        .SetVertexShader(prefix + "GroundFromAtmosphere" + vert)
        .SetFragmentShader(prefix + "GroundFromAtmosphere" + frag)
        .Build(m_GroundFromAtmosphere);

    pipelineBuilder
        .AddPushConstantRange()
	        .SetSize(sizeof(CameraMatricesPC))
	        .SetOffset(0)
	        .SetStageFlags(VK_SHADER_STAGE_VERTEX_BIT)
	        .EndRange()
        .AddDescriptorSet(m_vDescriptorSetsSpace.front())
        .SetVertexShader(prefix + "SpaceFromSpace" + vert)
        .SetFragmentShader(prefix + "SpaceFromSpace" + frag)
        .Build(m_SpaceFromSpace);

    pipelineBuilder
        .AddPushConstantRange()
	        .SetSize(sizeof(CameraMatricesPC))
	        .SetOffset(0)
	        .SetStageFlags(VK_SHADER_STAGE_VERTEX_BIT)
	        .EndRange()
        .AddDescriptorSet(m_vDescriptorSetsSpace.front())
        .SetVertexShader(prefix + "SpaceFromAtmosphere" + vert)
        .SetFragmentShader(prefix + "SpaceFromAtmosphere" + frag)
        .Build(m_SpaceFromAtmosphere);

    pipelineBuilder
        .AddPushConstantRange()
	        .SetSize(sizeof(CameraMatricesPC))
	        .SetOffset(0)
	        .SetStageFlags(VK_SHADER_STAGE_VERTEX_BIT)
	        .EndRange()
        .AddDescriptorSet(m_vDescriptorSetsSky.front())
        .SetVertexShader(prefix + "SkyFromSpace" + vert)
        .SetFragmentShader(prefix + "SkyFromSpace" + frag)
        .SetDepthTest(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS)
        .EnableColorBlend(0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD)
        .EnableAlphaBlend(0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD)
        .Build(m_SkyFromSpace);

    pipelineBuilder
        .AddPushConstantRange()
	        .SetSize(sizeof(CameraMatricesPC))
	        .SetOffset(0)
	        .SetStageFlags(VK_SHADER_STAGE_VERTEX_BIT)
	        .EndRange()
        .AddDescriptorSet(m_vDescriptorSetsSky.front())
        .SetVertexShader(prefix + "SkyFromAtmosphere" + vert)
        .SetFragmentShader(prefix + "SkyFromAtmosphere" + frag)
        .SetDepthTest(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS)
        .EnableColorBlend(0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD)
        .EnableAlphaBlend(0, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD)
        .Build(m_SkyFromAtmosphere);
}
void ashen::Renderer::CreateDescriptorSets()
{
    DescriptorPoolBuilder builder{ *m_pContext };
    auto count = m_pContext->GetSwapchainImageCount();
    builder
        .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count * 3 * 2)
        .SetMaxSets(count * 3)
        .SetFlags(0)
        .Build(m_DescriptorPool);

    m_vDescriptorSetsSky.resize(count);
    m_vDescriptorSetsGround.resize(count);
    m_vDescriptorSetsSpace.resize(count);
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
            .NewLayoutBinding()
	            .SetType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	            .SetCount(1)
	            .SetShaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
	            .EndLayoutBinding()
            .Allocate(m_DescriptorPool, m_vDescriptorSetsGround[i]);
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
            .Allocate(m_DescriptorPool, m_vDescriptorSetsSpace[i]);

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
            .AddBufferInfo(m_vUBOGround_FS[i], 0, sizeof(GroundFS))
            .WriteBuffers(m_vDescriptorSetsGround[i], 1)
            .Execute();

        writer
            .AddBufferInfo(m_vUBOSpace_VS[i], 0, sizeof(SpaceVS))
            .WriteBuffers(m_vDescriptorSetsSpace[i], 0)
            .Execute();
        writer
            .AddBufferInfo(m_vUBOSpace_FS[i], 0, sizeof(SpaceFS))
            .WriteBuffers(m_vDescriptorSetsSpace[i], 1)
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


    // Dynamic rendering info
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_pContext->GetSwapchainImageViews()[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { {0.f, 0.f, 0.f, 1.0f} };

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = m_vDepthImages[m_CurrentFrame].GetView();
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
void ashen::Renderer::RenderFrame()
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];

    auto camPos = m_pCamera->Position;
    auto camHeight = glm::length(camPos);
    CameraMatricesPC camMatrices{ m_pCamera->GetViewMatrix(), m_pCamera->GetProjectionMatrix() };

    // -- Space Objects --

    // -- Ground Objects --
    Pipeline* pGroundShader;
    if (camHeight >= m_OuterRadius) pGroundShader = &m_GroundFromSpace;
    else pGroundShader = &m_GroundFromAtmosphere;

    pGroundShader->Bind(cmd);
    m_pMeshFloor->Bind(cmd);
    vkCmdPushConstants(cmd, pGroundShader->GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(CameraMatricesPC), &camMatrices);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pGroundShader->GetLayoutHandle(), 0, 1,
							&m_vDescriptorSetsGround[m_CurrentFrame].GetHandle(), 0, nullptr);

    m_pMeshFloor->Draw(cmd);

    // -- Sky Objects --
    Pipeline* pSkyShader;
    if (camHeight >= m_OuterRadius) pSkyShader = &m_SkyFromSpace;
    else pSkyShader = &m_SkyFromAtmosphere;

    pSkyShader->Bind(cmd);
    m_pMeshSky->Bind(cmd);
    vkCmdPushConstants(cmd, pSkyShader->GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(CameraMatricesPC), &camMatrices);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pSkyShader->GetLayoutHandle(), 0, 1,
        &m_vDescriptorSetsSky[m_CurrentFrame].GetHandle(), 0, nullptr);

    m_pMeshSky->Draw(cmd);
}
void ashen::Renderer::EndFrame(uint32_t imageIndex) const
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];
    vkCmdEndRendering(cmd);

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


    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void ashen::Renderer::OnResize()
{
    auto size = m_pWindow->GetFramebufferSize();
    while (size.x == 0 || size.y == 0)
    {
        size = m_pWindow->GetFramebufferSize();
        m_pWindow->PollEvents();
    }

    VkDevice device = m_pContext->GetDevice();
    vkDeviceWaitIdle(device);
    m_pContext->RebuildSwapchain(size);
    CreateDepthResources(m_pContext->GetSwapchainExtent());

    for (const auto& sem : m_vImageAvailableSemaphores) vkDestroySemaphore(device, sem, nullptr);
    for (const auto& sem : m_vRenderFinishedSemaphores) vkDestroySemaphore(device, sem, nullptr);
    for (const auto& fence : m_vInFlightFences) vkDestroyFence(device, fence, nullptr);
    CreateSyncObjects();

    m_pCamera->AspectRatio = m_pWindow->GetAspectRatio();
}

void ashen::Renderer::RecordCommandBuffer(uint32_t imageIndex)
{
    SetupFrame(imageIndex);
    RenderFrame();
    EndFrame(imageIndex);
}
