// -- Standard Library --
#include <stdexcept>
#include <fstream>

// -- Ashen Includes --
#include "Pipeline.h"
#include "VulkanContext.h"

//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  Pipeline
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//--------------------------------------------------
//    Destructor
//--------------------------------------------------
ashen::Pipeline::~Pipeline()
{
    if (!m_pContext) return;
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


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  PushConstantRange
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//--------------------------------------------------
//    Build
//--------------------------------------------------
ashen::PushConstantRange& ashen::PushConstantRange::SetStageFlags(VkShaderStageFlags flags)
{
    m_Range.stageFlags = flags;
	return *this;
}
ashen::PushConstantRange& ashen::PushConstantRange::SetOffset(uint32_t offset)
{
    m_Range.offset = offset;
    return *this;
}
ashen::PushConstantRange& ashen::PushConstantRange::SetSize(uint32_t size)
{
    m_Range.size = size;
    return *this;
}
ashen::PipelineBuilder& ashen::PushConstantRange::EndRange() const
{
	return *m_pBuilder;
}

//--------------------------------------------------
//    Constructor
//--------------------------------------------------
ashen::PushConstantRange::PushConstantRange(PipelineBuilder& pipelineBuilder)
	: m_pBuilder(&pipelineBuilder)
{ }


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  Pipeline Builder
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//--------------------------------------------------
//    Constructor
//--------------------------------------------------
ashen::PipelineBuilder::PipelineBuilder(VulkanContext& context)
{
    m_pContext = &context;

    m_VertexInputInfo = {};
    m_VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    m_VertexInputInfo.vertexBindingDescriptionCount = 0;
    m_VertexInputInfo.pVertexBindingDescriptions = nullptr;
    m_VertexInputInfo.vertexAttributeDescriptionCount = 0;
    m_VertexInputInfo.pVertexAttributeDescriptions = nullptr;

    m_InputAssembly = {};
    m_InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    m_InputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_InputAssembly.primitiveRestartEnable = VK_FALSE;

    m_ViewportState = {};
    m_ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    m_ViewportState.viewportCount = 1;
    m_ViewportState.scissorCount = 1;

    m_RasterizerInfo = {};
    m_RasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    m_RasterizerInfo.depthClampEnable = VK_FALSE;
    m_RasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    m_RasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    m_RasterizerInfo.lineWidth = 1.0f;
    m_RasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    m_RasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    m_RasterizerInfo.depthBiasEnable = VK_FALSE;
    m_RasterizerInfo.depthBiasConstantFactor = VK_FALSE;
    m_RasterizerInfo.depthBiasSlopeFactor = VK_FALSE;

    m_MultiSamplingInfo = {};
    m_MultiSamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    m_MultiSamplingInfo.sampleShadingEnable = VK_FALSE;
    m_MultiSamplingInfo.minSampleShading = 0.0f;
    m_MultiSamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    m_DepthStencilInfo = {};
    m_DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    m_DepthStencilInfo.depthTestEnable = VK_TRUE;
    m_DepthStencilInfo.depthWriteEnable = VK_TRUE;
    m_DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    m_DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    m_DepthStencilInfo.minDepthBounds = 0.0f;
    m_DepthStencilInfo.maxDepthBounds = 1.0f;
    m_DepthStencilInfo.stencilTestEnable = VK_FALSE;
    m_DepthStencilInfo.front = {};
    m_DepthStencilInfo.back = {};

    m_vColorBlendAttachmentState.clear();

    m_ColorBlendCreateInfo = {};
    m_ColorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    m_ColorBlendCreateInfo.logicOpEnable = VK_FALSE;
    m_ColorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    m_ColorBlendCreateInfo.attachmentCount = 0;
    m_ColorBlendCreateInfo.pAttachments = nullptr;
    m_ColorBlendCreateInfo.blendConstants[0] = 0.0f;
    m_ColorBlendCreateInfo.blendConstants[1] = 0.0f;
    m_ColorBlendCreateInfo.blendConstants[2] = 0.0f;
    m_ColorBlendCreateInfo.blendConstants[3] = 0.0f;

    m_DynamicStateInfo = {};
    m_DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    m_DynamicStateInfo.dynamicStateCount = 0;
    m_DynamicStateInfo.pDynamicStates = nullptr;

    m_pNext = nullptr;
}

//--------------------------------------------------
//    Build
//--------------------------------------------------
// -- Push Constants --
ashen::PushConstantRange& ashen::PipelineBuilder::AddPushConstantRange()
{
    m_vPushConstantRanges.push_back(PushConstantRange(*this));
    return m_vPushConstantRanges.back();
}

// -- Descriptors --
ashen::PipelineBuilder& ashen::PipelineBuilder::AddDescriptorSet(DescriptorSet& descriptorSetLayout)
{
    m_vDescriptorLayouts.push_back(descriptorSetLayout.GetLayout());
    return *this;
}

// -- Shaders --
ashen::PipelineBuilder& ashen::PipelineBuilder::SetVertexShader(const std::string& vs)
{
    LoadShaderModule(vs, m_VertexShader);

    VkPipelineShaderStageCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderInfo.module = m_VertexShader;
    shaderInfo.pName = "main";
    shaderInfo.pSpecializationInfo = nullptr;

    m_vShaderInfo.push_back(shaderInfo);

    return *this;
}
ashen::PipelineBuilder& ashen::PipelineBuilder::SetFragmentShader(const std::string& fs)
{
    LoadShaderModule(fs, m_FragmentShader);

    VkPipelineShaderStageCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderInfo.module = m_FragmentShader;
    shaderInfo.pName = "main";
    shaderInfo.pSpecializationInfo = nullptr;

    m_vShaderInfo.push_back(shaderInfo);

    return *this;
}

// -- Vertex --
ashen::PipelineBuilder& ashen::PipelineBuilder::SetVertexBindingDesc(const VkVertexInputBindingDescription& desc)
{
    m_VertexInputInfo.vertexBindingDescriptionCount = 1;
    m_VertexInputInfo.pVertexBindingDescriptions = &desc;
    return *this;
}
ashen::PipelineBuilder& ashen::PipelineBuilder::SetVertexAttributeDesc(const std::vector<VkVertexInputAttributeDescription>& attr)
{
    m_VertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr.size());
    m_VertexInputInfo.pVertexAttributeDescriptions = attr.data();
    return *this;
}

// -- Other --
ashen::PipelineBuilder& ashen::PipelineBuilder::SetPrimitiveTopology(VkPrimitiveTopology topology)
{
    m_InputAssembly.topology = topology;
    return *this;
}
ashen::PipelineBuilder& ashen::PipelineBuilder::SetCullMode(VkCullModeFlags cullMode)
{
    m_RasterizerInfo.cullMode = cullMode;
    return *this;
}
ashen::PipelineBuilder& ashen::PipelineBuilder::SetPolygonMode(VkPolygonMode polyMode)
{
    m_RasterizerInfo.polygonMode = polyMode;
    return *this;
}
ashen::PipelineBuilder& ashen::PipelineBuilder::SetFrontFace(VkFrontFace front)
{
    m_RasterizerInfo.frontFace = front;
    return *this;
}
ashen::PipelineBuilder& ashen::PipelineBuilder::AddDynamicState(VkDynamicState dynamicState)
{
    m_vDynamicStates.push_back(dynamicState);
    m_DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(m_vDynamicStates.size());
    m_DynamicStateInfo.pDynamicStates = m_vDynamicStates.data();
    return *this;
}
ashen::PipelineBuilder& ashen::PipelineBuilder::SetupDynamicRendering(VkPipelineRenderingCreateInfo& dynamicRenderInfo)
{
    m_pNext = &dynamicRenderInfo;
    for (uint32_t i{}; i < dynamicRenderInfo.colorAttachmentCount; ++i)
    {
        m_vColorBlendAttachmentState.push_back(
            {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask =
                        VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT |
                        VK_COLOR_COMPONENT_A_BIT
            }
        );
    }
    return *this;
}

// -- Depth testing --
ashen::PipelineBuilder& ashen::PipelineBuilder::SetDepthTest(VkBool32 depthRead, VkBool32 depthWrite,	VkCompareOp compareOp)
{
    m_DepthStencilInfo.depthWriteEnable = depthWrite;
    m_DepthStencilInfo.depthTestEnable = depthRead;
    m_DepthStencilInfo.depthCompareOp = compareOp;
    return *this;
}

// -- Build --
void ashen::PipelineBuilder::Build(Pipeline& pipeline)
{
    // -- Layout --
    std::vector<VkPushConstantRange> vVulkanRanges{};
    vVulkanRanges.reserve(m_vPushConstantRanges.size());
    for (const auto& range : m_vPushConstantRanges)
        vVulkanRanges.push_back(range.m_Range);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_vDescriptorLayouts.size());
    pipelineLayoutInfo.pSetLayouts = m_vDescriptorLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(vVulkanRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = vVulkanRanges.empty() ? nullptr : vVulkanRanges.data();

    if (vkCreatePipelineLayout(m_pContext->GetDevice(), &pipelineLayoutInfo, nullptr, &pipeline.m_Layout) != VK_SUCCESS)
        throw std::runtime_error("failed to create Pipeline Layout!");

    // -- Pipeline --
    m_ColorBlendCreateInfo.attachmentCount = static_cast<uint32_t>(m_vColorBlendAttachmentState.size());
    m_ColorBlendCreateInfo.pAttachments = m_vColorBlendAttachmentState.data();
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = m_pNext;
    pipelineInfo.stageCount = static_cast<uint32_t>(m_vShaderInfo.size());
    pipelineInfo.pStages = m_vShaderInfo.data();
    pipelineInfo.pVertexInputState = &m_VertexInputInfo;
    pipelineInfo.pInputAssemblyState = &m_InputAssembly;
    pipelineInfo.pViewportState = &m_ViewportState;
    pipelineInfo.pRasterizationState = &m_RasterizerInfo;
    pipelineInfo.pMultisampleState = &m_MultiSamplingInfo;
    pipelineInfo.pDepthStencilState = &m_DepthStencilInfo;
    pipelineInfo.pColorBlendState = &m_ColorBlendCreateInfo;
    pipelineInfo.pDynamicState = &m_DynamicStateInfo;
    pipelineInfo.layout = pipeline.m_Layout;
    pipelineInfo.renderPass = nullptr;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_pContext->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline.m_Pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Graphics Pipeline!");

    pipeline.m_pContext = m_pContext;

    m_vPushConstantRanges.clear();
    m_vDescriptorLayouts.clear();
    m_vShaderInfo.clear();
    if (m_VertexShader != VK_NULL_HANDLE) vkDestroyShaderModule(m_pContext->GetDevice(), m_VertexShader, nullptr);
    if (m_FragmentShader != VK_NULL_HANDLE) vkDestroyShaderModule(m_pContext->GetDevice(), m_FragmentShader, nullptr);
}


//--------------------------------------------------
//    Helper
//--------------------------------------------------
void ashen::PipelineBuilder::LoadShaderModule(const std::string& filename, VkShaderModule& shaderMod) const
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
