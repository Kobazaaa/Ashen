#ifndef ASHEN_PIPELINE_H
#define ASHEN_PIPELINE_H

// -- Vulkan Includes --
#include <vulkan/vulkan.h>

// -- Standard Library --
#include <vector>

// -- Forward Declarations --
namespace ashen
{
	class ShaderModule;
	class DescriptorSetLayout;
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
		explicit Pipeline(VulkanContext& context, 
						const std::vector<VkPushConstantRange>& pcr, 
						const std::vector<VkDescriptorSetLayout>& dcl,
						const std::string& vs, const std::string& fs);
		~Pipeline();

		Pipeline(const Pipeline& other) = delete;
		Pipeline(Pipeline&& other) noexcept = delete;
		Pipeline& operator=(const Pipeline& other) = delete;
		Pipeline& operator=(Pipeline&& other) noexcept = delete;

		//--------------------------------------------------
		//    Functionality
		//--------------------------------------------------
		void Bind(VkCommandBuffer cmd) const;

		//--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
		const VkPipeline& GetPipelineHandle() const;
		const VkPipelineLayout& GetLayoutHandle() const;

	private:
		void LoadShaderModule(const std::string& filename, VkShaderModule& shaderMod) const;

		VkPipeline m_Pipeline;
		VkPipelineLayout m_Layout;
		VulkanContext* m_pContext{};
	};
}

#endif // ASHEN_PIPELINE_H