#ifndef ASHEN_PIPELINE_H
#define ASHEN_PIPELINE_H

// -- Vulkan Includes --
#include <vulkan/vulkan.h>

// -- Standard Library --
#include <vector>

// -- Ashen Includes --
#include "Descriptors.h"

// -- Forward Declarations --
namespace ashen
{
	class VulkanContext;
}

namespace ashen
{
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  Pipeline
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class Pipeline final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		Pipeline() = default;
		~Pipeline();

		Pipeline(const Pipeline& other) = delete;
		Pipeline(Pipeline&& other) noexcept = default;
		Pipeline& operator=(const Pipeline& other) = delete;
		Pipeline& operator=(Pipeline&& other) noexcept = default;

		//--------------------------------------------------
		//    Functionality
		//--------------------------------------------------
		void Bind(VkCommandBuffer cmd) const;
		void Destroy();

		//--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
		const VkPipeline& GetPipelineHandle() const;
		const VkPipelineLayout& GetLayoutHandle() const;

	private:

		VkPipeline m_Pipeline{};
		VkPipelineLayout m_Layout{};
		VulkanContext* m_pContext{};

		friend class PipelineBuilder;
	};


	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  PushConstantRange
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class PushConstantRange final
	{
	public:
		//--------------------------------------------------
		//    Build
		//--------------------------------------------------
		PushConstantRange& SetStageFlags(VkShaderStageFlags flags);
		PushConstantRange& SetOffset(uint32_t offset);
		PushConstantRange& SetSize(uint32_t size);
		PipelineBuilder& EndRange() const;

	private:
		//--------------------------------------------------
		//    Constructor
		//--------------------------------------------------
		PushConstantRange(PipelineBuilder& pipelineBuilder);

		VkPushConstantRange m_Range{};
		PipelineBuilder* m_pBuilder{};

		friend class PipelineBuilder;
	};


	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  Pipeline Builder
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class PipelineBuilder final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		PipelineBuilder(VulkanContext& context);

		//--------------------------------------------------
		//    Build
		//--------------------------------------------------
		// -- Push Constants --
		PushConstantRange& AddPushConstantRange();

		// -- Descriptors --
		PipelineBuilder& AddDescriptorSet(DescriptorSet& descriptorSetLayout);

		// -- Shaders --
		PipelineBuilder& SetVertexShader(const std::string& vs);
		PipelineBuilder& SetFragmentShader(const std::string& fs);

		// -- Vertex --
		PipelineBuilder& SetVertexBindingDesc(const VkVertexInputBindingDescription& desc);
		PipelineBuilder& SetVertexAttributeDesc(const std::vector<VkVertexInputAttributeDescription>& attr);

		// -- Blending --
		PipelineBuilder& EnableColorBlend(uint32_t attachment, VkBlendFactor src, VkBlendFactor dst, VkBlendOp op);
		PipelineBuilder& EnableAlphaBlend(uint32_t attachment, VkBlendFactor src, VkBlendFactor dst, VkBlendOp op);

		// -- Other --
		PipelineBuilder& SetPrimitiveTopology(VkPrimitiveTopology topology);
		PipelineBuilder& SetCullMode(VkCullModeFlags cullMode);
		PipelineBuilder& SetPolygonMode(VkPolygonMode polyMode);
		PipelineBuilder& SetFrontFace(VkFrontFace front);
		PipelineBuilder& AddDynamicState(VkDynamicState dynamicState);
		PipelineBuilder& SetupDynamicRendering(VkPipelineRenderingCreateInfo& dynamicRenderInfo);

		// -- Depth Testing --
		PipelineBuilder& SetDepthTest(VkBool32 depthRead, VkBool32 depthWrite, VkCompareOp compareOp);

		// -- Build --
		void Build(Pipeline& pipeline);

	private:
		void LoadShaderModule(const std::string& filename, VkShaderModule& shaderMod) const;
		void*		m_pNext;

		VkPipelineVertexInputStateCreateInfo				m_VertexInputInfo{};
		VkPipelineInputAssemblyStateCreateInfo				m_InputAssembly{};
		VkPipelineViewportStateCreateInfo					m_ViewportState{};
		VkPipelineRasterizationStateCreateInfo				m_RasterizerInfo{};
		VkPipelineMultisampleStateCreateInfo				m_MultiSamplingInfo{};
		VkPipelineDepthStencilStateCreateInfo				m_DepthStencilInfo{};
		std::vector<VkPipelineColorBlendAttachmentState>	m_vColorBlendAttachmentState{};
		VkPipelineColorBlendStateCreateInfo					m_ColorBlendCreateInfo{};
		VkPipelineDynamicStateCreateInfo					m_DynamicStateInfo{};

		std::vector<VkDynamicState>							m_vDynamicStates;
		std::vector<VkPipelineShaderStageCreateInfo>		m_vShaderInfo;
		std::vector<VkSpecializationMapEntry >				m_vShaderSpecializationEntries;
		std::vector<VkSpecializationInfo>					m_vSpecializationInfo;
		std::vector<PushConstantRange>						m_vPushConstantRanges;
		std::vector<VkDescriptorSetLayout>					m_vDescriptorLayouts;

		VkShaderModule										m_VertexShader{};
		VkShaderModule										m_FragmentShader{};
		VulkanContext*										m_pContext{};
	};


}

#endif // ASHEN_PIPELINE_H