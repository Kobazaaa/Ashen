// -- Ashen Includes --
#include "Image.h"
#include "Buffer.h"
#include "VulkanContext.h"

//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  Image	
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Image::~Image()
{
	if (!m_pContext) return;
	vkDestroyImageView(m_pContext->GetDevice(), m_ImageView, nullptr);
	vkDestroyImage(m_pContext->GetDevice(), m_Image, nullptr);
	vkFreeMemory(m_pContext->GetDevice(), m_ImageMemory, nullptr);
}

//--------------------------------------------------
//    Helpers
//--------------------------------------------------
VkFormat ashen::Image::FindSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (const VkFormat format : candidates)
	{
		VkFormatProperties props{};
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features ||
			tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return format;
	}

	throw std::runtime_error("Failed to find Supported Format!");
}


//--------------------------------------------------
//    Accessors & Mutators
//--------------------------------------------------
const VkImage& ashen::Image::GetHandle() const
{
	return m_Image;
}
const VkImageView& ashen::Image::GetView() const
{
	return m_ImageView;
}

VkFormat ashen::Image::GetFormat() const
{
	return m_ImageInfo.format;
}
VkExtent3D ashen::Image::GetExtent() const
{
	return m_ImageInfo.extent;
}

VkImageLayout ashen::Image::GetCurrentLayout() const
{
	return m_CurrentLayout;
}
bool ashen::Image::HasStencilComponent() const
{
	switch (m_ImageInfo.format)
	{
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
		return true;
	default:
		return false;
	}
}
bool ashen::Image::HasDepthComponent() const
{
	switch (m_ImageInfo.format)
	{
	case VK_FORMAT_D16_UNORM:
	case VK_FORMAT_X8_D24_UNORM_PACK32:
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		return true;
	default:
		return false;
	}
}


//--------------------------------------------------
//    Commands
//--------------------------------------------------
void ashen::Image::TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage)
{
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.pNext = nullptr;

	barrier.image = m_Image;
	barrier.oldLayout = m_CurrentLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = srcAccess;
	barrier.srcStageMask = srcStage;

	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstAccessMask = dstAccess;
	barrier.dstStageMask = dstStage;

	barrier.subresourceRange.aspectMask = 0;
	if (HasDepthComponent())
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (HasStencilComponent()) barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.dependencyFlags = 0;
	dependencyInfo.pNext = nullptr;
	dependencyInfo.memoryBarrierCount = 0;
	dependencyInfo.pMemoryBarriers = nullptr;
	dependencyInfo.bufferMemoryBarrierCount = 0;
	dependencyInfo.pBufferMemoryBarriers = nullptr;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;

	vkCmdPipelineBarrier2(cmd, &dependencyInfo);

	m_CurrentLayout = newLayout;
}
void ashen::Image::InsertBarrier(VkCommandBuffer cmd, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 srcStage,	VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage)
{
	TransitionLayout(cmd, m_CurrentLayout, srcAccess, srcStage, dstAccess, dstStage);
}


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  ImageBuilder	
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------

ashen::ImageBuilder::ImageBuilder(VulkanContext& context)
{
	m_pContext = &context;

	m_ImageInfo = {};
	m_ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	m_ImageInfo.imageType = VK_IMAGE_TYPE_2D;
	m_ImageInfo.extent.width = 0;
	m_ImageInfo.extent.height = 0;
	m_ImageInfo.extent.depth = 1;
	m_ImageInfo.mipLevels = 1;
	m_ImageInfo.arrayLayers = 1;
	m_ImageInfo.format = VK_FORMAT_UNDEFINED;
	m_ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	m_ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_ImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	m_ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	m_ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	m_ImageInfo.flags = 0;

	m_PreMadeImage = VK_NULL_HANDLE;
	m_UseInitialData = false;
	m_pData = nullptr;
	m_InitDataSize = 0;
	m_InitDataHeight = 0;
	m_InitDataWidth = 0;
	m_InitDataOffset = 0;
	m_FinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

//--------------------------------------------------
//    Builder
//--------------------------------------------------
// -- Size --
ashen::ImageBuilder& ashen::ImageBuilder::SetWidth(uint32_t width)
{
	m_ImageInfo.extent.width = width;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetHeight(uint32_t height)
{
	m_ImageInfo.extent.height = height;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetDepth(uint32_t depth)
{
	m_ImageInfo.extent.depth = depth;
	return *this;
}

// -- Info --
ashen::ImageBuilder& ashen::ImageBuilder::SetFormat(VkFormat format)
{
	m_ImageInfo.format = format;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetTiling(VkImageTiling tiling)
{
	m_ImageInfo.tiling = tiling;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetUsageFlags(VkImageUsageFlags usage)
{
	m_ImageInfo.usage = usage;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetCreateFlags(VkImageCreateFlags flags)
{
	m_ImageInfo.flags |= flags;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetAspectFlags(VkImageAspectFlags aspectFlags)
{
	m_AspectFlags = aspectFlags;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetViewType(VkImageViewType viewType)
{
	m_ViewType = viewType;
	return *this;
}

// -- Data --
ashen::ImageBuilder& ashen::ImageBuilder::InitialData(void* data, uint32_t offset, uint32_t width, uint32_t height,	uint32_t dataSize, VkImageLayout finalLayout)
{
	m_UseInitialData = true;
	m_pData = data;
	m_InitDataOffset = offset;
	m_InitDataSize = dataSize;
	m_InitDataHeight = height;
	m_InitDataWidth = width;
	m_ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (m_ImageInfo.mipLevels > 1)
		m_ImageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	m_FinalLayout = finalLayout;
	return *this;
}
ashen::ImageBuilder& ashen::ImageBuilder::SetPreMadeImage(VkImage image)
{
	m_PreMadeImage = image;
	return *this;
}

// -- Build --
void ashen::ImageBuilder::Build(Image& image) const
{
	image.m_pContext = m_pContext;
	image.m_ImageInfo = m_ImageInfo;
	image.m_CurrentLayout = m_ImageInfo.initialLayout;
	image.m_Image = m_PreMadeImage;
	if (image.m_Image == VK_NULL_HANDLE)
	{
		if (vkCreateImage(m_pContext->GetDevice(), &m_ImageInfo, nullptr, &image.m_Image) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Image!");
	}

	if (m_UseInitialData)
	{
		Buffer stagingBuffer;
		BufferAllocator stagingAllocator{ *m_pContext };
		stagingAllocator
			.SetUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.HostAccess(true)
			.SetSize(m_InitDataSize)
			.Allocate(stagingBuffer);

		void* data = nullptr;
		vkMapMemory(m_pContext->GetDevice(), stagingBuffer.GetMemoryHandle(), 0, VK_WHOLE_SIZE, 0, &data);
		memcpy(data, m_pData, m_InitDataSize);
		vkUnmapMemory(m_pContext->GetDevice(), stagingBuffer.GetMemoryHandle());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_pContext->GetCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_pContext->GetDevice(), &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		{
			image.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				0, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			stagingBuffer.CopyToImage(commandBuffer, image, VkExtent3D{ m_InitDataWidth, m_InitDataHeight, 1 });
		}
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(m_pContext->GetQueue(vkb::QueueType::graphics), 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_pContext->GetQueue(vkb::QueueType::graphics));

		vkFreeCommandBuffers(m_pContext->GetDevice(), m_pContext->GetCommandPool(), 1, &commandBuffer);
	}

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image.m_Image;
	viewInfo.viewType = m_ViewType;
	viewInfo.format = m_ImageInfo.format;
	viewInfo.subresourceRange.aspectMask = m_AspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_pContext->GetDevice(), &viewInfo, nullptr, &image.m_ImageView) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Image View!");
}
