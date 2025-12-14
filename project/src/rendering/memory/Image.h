#ifndef ASHEN_IMAGE_H
#define ASHEN_IMAGE_H

// -- Standard Library --
#include <vector>

// -- Vulkan Includes --
#include <vulkan/vulkan.h>

// -- Forward Declarations --
namespace ashen
{
	class VulkanContext;
}

namespace ashen
{
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  Image	
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class Image final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		explicit Image() = default;
		~Image();

		Image(const Image& other) = delete;
		Image(Image&& other) noexcept = default;
		Image& operator=(const Image& other) = delete;
		Image& operator=(Image&& other) noexcept = default;

		//--------------------------------------------------
		//    Helpers
		//--------------------------------------------------
		static VkFormat FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		//--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
		const VkImage&		GetHandle()						const;
		const VkImageView&	GetView()						const;

		VkFormat			GetFormat()						const;
		VkExtent3D			GetExtent()						const;
		VkImageLayout		GetCurrentLayout()				const;

		bool				HasStencilComponent()			const;
		bool				HasDepthComponent()				const;

		//--------------------------------------------------
		//    Commands
		//--------------------------------------------------
		void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout,
			VkAccessFlags2 srcAccess, VkPipelineStageFlags2 srcStage,
			VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);
		void InsertBarrier(VkCommandBuffer cmd,
			VkAccessFlags2 srcAccess, VkPipelineStageFlags2 srcStage,
			VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage);

	private:
		VkImage			m_Image			{ VK_NULL_HANDLE };
		VkImageView		m_ImageView		{ VK_NULL_HANDLE };
		VkDeviceMemory	m_ImageMemory	{ VK_NULL_HANDLE };

		VkImageLayout m_CurrentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
		VkImageCreateInfo m_ImageInfo{ };

		VulkanContext* m_pContext{};

		friend class ImageBuilder;
	};


	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  ImageBuilder	
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class ImageBuilder final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		ImageBuilder(VulkanContext& context);

		//--------------------------------------------------
		//    Builder
		//--------------------------------------------------
		// -- Size --
		ImageBuilder& SetWidth(uint32_t width);
		ImageBuilder& SetHeight(uint32_t height);
		ImageBuilder& SetDepth(uint32_t depth);

		// -- Info --
		ImageBuilder& SetFormat(VkFormat format);
		ImageBuilder& SetTiling(VkImageTiling tiling);
		ImageBuilder& SetUsageFlags(VkImageUsageFlags usage);
		ImageBuilder& SetCreateFlags(VkImageCreateFlags flags);
		ImageBuilder& SetAspectFlags(VkImageAspectFlags aspectFlags);
		ImageBuilder& SetViewType(VkImageViewType viewType);

		// -- Data --
		ImageBuilder& InitialData(void* data, uint32_t offset, uint32_t width, uint32_t height, uint32_t dataSize, VkImageLayout finalLayout);
		ImageBuilder& SetPreMadeImage(VkImage image);

		// -- Build --
		void Build(Image& image) const;

	private:
		bool m_UseInitialData{};
		void* m_pData{};
		uint32_t m_InitDataSize{};
		uint32_t m_InitDataWidth{};
		uint32_t m_InitDataHeight{};
		uint32_t m_InitDataOffset{};

		VkImageLayout m_FinalLayout{};

		VkImageAspectFlags m_AspectFlags{};
		VkImageViewType m_ViewType{};

		VkImage m_PreMadeImage{};
		VkImageCreateInfo m_ImageInfo{};
		VulkanContext* m_pContext{};
	};
}

#endif // ASHEN_IMAGE_H