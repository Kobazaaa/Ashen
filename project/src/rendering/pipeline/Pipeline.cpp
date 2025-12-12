// -- Standard Library --
#include <stdexcept>
#include <fstream>

// -- Ashen Includes --
#include "Pipeline.h"
#include "VulkanContext.h"
#include "Vertex.h"


ashen::Pipeline::Pipeline(VulkanContext& context,
							const std::vector<VkPushConstantRange>& pcr, const std::vector<VkDescriptorSetLayout>& dcl,
							const std::string& vs, const std::string& fs)
{
    m_pContext = &context;

    // -- Pipeline Layout --
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(dcl.size());
    pipelineLayoutInfo.pSetLayouts = dcl.empty() ? nullptr : dcl.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pcr.size());
    pipelineLayoutInfo.pPushConstantRanges = pcr.empty() ? nullptr : pcr.data();

    if (vkCreatePipelineLayout(m_pContext->GetDevice(), &pipelineLayoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
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
    VkPipelineShaderStageCreateInfo shaderStages[2] = { };
    VkShaderModule vertShader{ VK_NULL_HANDLE };
    VkShaderModule fragShader{ VK_NULL_HANDLE };

    if (!vs.empty())
    {
	    LoadShaderModule(vs, vertShader);

	    VkPipelineShaderStageCreateInfo shaderInfoFragVert{};
        shaderInfoFragVert.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderInfoFragVert.stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderInfoFragVert.module = vertShader;
        shaderInfoFragVert.pName = "main";
        shaderInfoFragVert.pSpecializationInfo = nullptr;
        shaderStages[0] = shaderInfoFragVert;
    }

    if (!fs.empty())
    {
	    LoadShaderModule(fs, fragShader);

	    VkPipelineShaderStageCreateInfo shaderInfoFragFrag{};
        shaderInfoFragFrag.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderInfoFragFrag.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderInfoFragFrag.module = fragShader;
        shaderInfoFragFrag.pName = "main";
        shaderInfoFragFrag.pSpecializationInfo = nullptr;
        shaderStages[1] = shaderInfoFragFrag;
    }


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
    rasterizer.cullMode = VK_CULL_MODE_NONE;
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
    pipelineInfo.layout = m_Layout;

    vkCreateGraphicsPipelines(m_pContext->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline);

    // -- Destroy Shaders --
    if(vertShader != VK_NULL_HANDLE) vkDestroyShaderModule(m_pContext->GetDevice(), vertShader, nullptr);
    if(fragShader != VK_NULL_HANDLE) vkDestroyShaderModule(m_pContext->GetDevice(), fragShader, nullptr);
}
ashen::Pipeline::~Pipeline()
{
	vkDestroyPipelineLayout(m_pContext->GetDevice(), m_Layout, nullptr);
	vkDestroyPipeline(m_pContext->GetDevice(), m_Pipeline, nullptr);
}


//--------------------------------------------------
//    Functionality
//--------------------------------------------------
void ashen::Pipeline::Bind(VkCommandBuffer cmd) const
{
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
}

//--------------------------------------------------
//    Accessors & Mutators
//--------------------------------------------------
const VkPipeline& ashen::Pipeline::GetPipelineHandle() const
{
	return m_Pipeline;
}
const VkPipelineLayout& ashen::Pipeline::GetLayoutHandle() const
{
	return m_Layout;
}


//--------------------------------------------------
//    Helpers
//--------------------------------------------------
void ashen::Pipeline::LoadShaderModule(const std::string& filename, VkShaderModule& shaderMod) const
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
