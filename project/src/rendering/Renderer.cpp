// -- Standard Library --
#include <fstream>

// -- Ashen Includes --
#include "Renderer.h"

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Renderer::Renderer(VulkanContext* pContext, Window* pWindow)
	: m_pContext(pContext)
	, m_pWindow(pWindow)
{
    m_pCamera = std::make_unique<Camera>(pWindow);

	CreateSyncObjects();
	CreateBuffers();
    CreatePipeline();
	CreateCommandBuffers();
}
ashen::Renderer::~Renderer()
{
    VkDevice device = m_pContext->GetDevice();
    vkDeviceWaitIdle(device);

	vkFreeCommandBuffers(device, m_CommandPool, static_cast<uint32_t>(m_vCommandBuffers.size()), m_vCommandBuffers.data());
	vkDestroyCommandPool(device, m_CommandPool, nullptr);

    vkDestroyPipeline(device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);

    vkDestroyBuffer(device, m_VertexBuffer, nullptr);
    vkFreeMemory(device, m_VertexBufferMemory, nullptr);
    vkDestroyBuffer(device, m_IndexBuffer, nullptr);
    vkFreeMemory(device, m_IndexBufferMemory, nullptr);

	for (const auto& sem : m_vImageAvailableSemaphores) vkDestroySemaphore(device, sem, nullptr);
	for (const auto& sem : m_vRenderFinishedSemaphores) vkDestroySemaphore(device, sem, nullptr);
	for (const auto& fence : m_vInFlightFences) vkDestroyFence(device, fence, nullptr);
}


//--------------------------------------------------
//    Functionality
//--------------------------------------------------
void ashen::Renderer::Update()
{
    m_pCamera->Update();
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


//--------------------------------------------------
//    Helpers
//--------------------------------------------------
void ashen::Renderer::OnResize() const
{
    auto size = m_pWindow->GetFramebufferSize();
    while (size.x == 0 || size.y == 0)
    {
        size = m_pWindow->GetFramebufferSize();
        m_pWindow->PollEvents();
    }

    m_pContext->RebuildSwapchain(size);
    m_pCamera->AspectRatio = m_pWindow->GetAspectRatio();
}
void ashen::Renderer::RecordCommandBuffer(uint32_t imageIndex)
{
    VkCommandBuffer cmd = m_vCommandBuffers[m_CurrentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer!");
    }

    static bool firstRun = true;
    VkImageMemoryBarrier presentBarrier{};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.oldLayout = firstRun ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.srcAccessMask = VK_ACCESS_NONE;
    presentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.image = m_pContext->GetSwapchainImages()[imageIndex];
    presentBarrier.subresourceRange = 
    {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, 1,
        0, 1
    };
    firstRun = true;

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
    colorAttachment.clearValue.color = { {0.1f, 0.2f, 0.3f, 1.0f} };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = { 0, 0 };
    renderingInfo.renderArea.extent = m_pContext->GetSwapchainExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_pContext->GetSwapchainExtent().width);
    viewport.height = static_cast<float>(m_pContext->GetSwapchainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_pContext->GetSwapchainExtent();

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    struct PC
    {
        glm::mat4 view;
        glm::mat4 proj;
    } pcvs;
    pcvs.view = m_pCamera->GetViewMatrix();
    pcvs.proj = m_pCamera->GetProjectionMatrix();
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 2 * sizeof(glm::mat4), &pcvs);

    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_vIndices.size()), 1, 0, 0, 0);

    vkCmdEndRendering(cmd);


    presentBarrier = {};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    presentBarrier.dstAccessMask = 0;
    presentBarrier.image = m_pContext->GetSwapchainImages()[imageIndex];
    presentBarrier.subresourceRange = 
    {
        VK_IMAGE_ASPECT_COLOR_BIT,
        0, 1,
        0, 1
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
void ashen::Renderer::CreateBuffers()
{
    auto vBufferSize = sizeof(m_vVertices[0]) * m_vVertices.size();
    CreateBuffer(vBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_VertexBuffer, m_VertexBufferMemory);
    auto iBufferSize = sizeof(m_vIndices[0]) * m_vIndices.size();
    CreateBuffer(iBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_IndexBuffer, m_IndexBufferMemory);

    UploadToBuffer(m_VertexBufferMemory, (void*)m_vVertices.data(), vBufferSize);
    UploadToBuffer(m_IndexBufferMemory, (void*)m_vIndices.data(), vBufferSize);
}
void ashen::Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_pContext->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_pContext->GetDevice(), buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_pContext->GetPhysicalDevice(), &memProperties);
    uint32_t outcome = 0;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            outcome = i;
            break;
        }
    }
    allocInfo.memoryTypeIndex = outcome;

    if (vkAllocateMemory(m_pContext->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate buffer memory");

    vkBindBufferMemory(m_pContext->GetDevice(), buffer, bufferMemory, 0);
}
void ashen::Renderer::UploadToBuffer(const VkDeviceMemory& bufferMemory, void* data, VkDeviceSize size) const
{
    void* pData;
    vkMapMemory(m_pContext->GetDevice(), bufferMemory, 0, VK_WHOLE_SIZE, 0, &pData);
    memcpy(pData, data, size);
    vkUnmapMemory(m_pContext->GetDevice(), bufferMemory);
}

void ashen::Renderer::CreateCommandBuffers()
{
    VkDevice device = m_pContext->GetDevice();

    // -- Command Pool --
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_pContext->GetQueueIndex(vkb::QueueType::graphics);

    vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool);

    // -- Command Buffers --
    m_vCommandBuffers.resize(m_pContext->GetSwapchainImageCount());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
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

void ashen::Renderer::CreatePipeline()
{
    // -- Vertex Push Constant Range --
    VkPushConstantRange pushConstantRangeVertex{};
    pushConstantRangeVertex.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRangeVertex.size = 2 * sizeof(glm::mat4);
    pushConstantRangeVertex.offset = 0;

    // -- Pipeline Layout --
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRangeVertex;

    if (vkCreatePipelineLayout(m_pContext->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("failed to create Pipeline Layout!");

    // -- Vertex Info --
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    auto bindingDescription = Vertex::GetBindingDescription();
    auto attrDescriptions = Vertex::GetAttributeDescriptions();
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescriptions.data();

    // -- Shaders --
    VkShaderModule vertShader{};
    LoadShaderModule("shaders/triangle.vert.spv", vertShader);

    VkPipelineShaderStageCreateInfo shaderInfoVert{};
    shaderInfoVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfoVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderInfoVert.module = vertShader;
    shaderInfoVert.pName = "main";
    shaderInfoVert.pSpecializationInfo = nullptr;

    VkShaderModule fragShader{};
    LoadShaderModule("shaders/triangle.frag.spv", fragShader);
    VkPipelineShaderStageCreateInfo shaderInfoFrag{};
    shaderInfoFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfoFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderInfoFrag.module = fragShader;
    shaderInfoFrag.pName = "main";
    shaderInfoFrag.pSpecializationInfo = nullptr;

    VkPipelineShaderStageCreateInfo shaderStages[] = { shaderInfoVert, shaderInfoFrag };

    // -- Dynamic Rendering Info --
    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    VkFormat swapchainFormat = m_pContext->GetSwapchainFormat();
    pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.pScissors = nullptr;

    // -- Input Assembly --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // -- Viewport --
    VkPipelineDynamicStateCreateInfo dynamicState{};
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // -- Rasterizer --
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // -- Multisampling --
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // -- Color Blending Attachment --
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    // -- Color Blending --
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // -- Depth Stencil --
    VkPipelineDepthStencilStateCreateInfo depthInfo{};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_FALSE;
	depthInfo.depthWriteEnable = VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthInfo.depthBoundsTestEnable = VK_FALSE;
	depthInfo.minDepthBounds = 0.0f;
	depthInfo.maxDepthBounds = 1.0f;
	depthInfo.stencilTestEnable = VK_FALSE;
	depthInfo.front = {};
	depthInfo.back = {};

    // -- Pipeline --
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthInfo;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;

    vkCreateGraphicsPipelines(m_pContext->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);

    // -- Destroy Shaders --
    vkDestroyShaderModule(m_pContext->GetDevice(), vertShader, nullptr);
    vkDestroyShaderModule(m_pContext->GetDevice(), fragShader, nullptr);
}

void ashen::Renderer::LoadShaderModule(const std::string& filename, VkShaderModule& shaderMod) const
{
    // -- Load & Read Shader --
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filename);

    size_t fileSize = (size_t)file.tellg();

    std::vector<char> vCode{};
    vCode.resize(fileSize);

    file.seekg(0);
    file.read(vCode.data(), static_cast<uint32_t>(fileSize));

    file.close();

    // -- Create Shader Module --
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = vCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vCode.data());

	if (vkCreateShaderModule(m_pContext->GetDevice(), &createInfo, nullptr, &shaderMod) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Shader Module!");

    vCode.clear();
}
