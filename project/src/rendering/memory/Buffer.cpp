// -- Ashen Includes --
#include "Buffer.h"
#include "Image.h"
#include "VulkanContext.h"

//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  Buffer	
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Buffer::~Buffer()
{
	if (!m_pContext) return;
	vkDestroyBuffer(m_pContext->GetDevice(), m_Buffer, nullptr);
	vkFreeMemory(m_pContext->GetDevice(), m_Memory, nullptr);
}

//--------------------------------------------------
//    Accessors & Mutators
//--------------------------------------------------
const VkBuffer& ashen::Buffer::GetHandle() const
{
	return m_Buffer;
}
const VkDeviceMemory& ashen::Buffer::GetMemoryHandle() const
{
	return m_Memory;
}
VkDeviceSize ashen::Buffer::Size() const
{
	return m_Size;
}

//--------------------------------------------------
//    Commands
//--------------------------------------------------
void ashen::Buffer::InsertBarrier(VkCommandBuffer cmd, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage) const
{
	VkBufferMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
	barrier.pNext = nullptr;

	barrier.buffer = m_Buffer;
	barrier.size = VK_WHOLE_SIZE;
	barrier.offset = 0;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcAccessMask = srcAccess;
	barrier.srcStageMask = srcStage;

	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstAccessMask = dstAccess;
	barrier.dstStageMask = dstStage;

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.dependencyFlags = 0;
	dependencyInfo.pNext = nullptr;
	dependencyInfo.memoryBarrierCount = 0;
	dependencyInfo.pMemoryBarriers = nullptr;
	dependencyInfo.bufferMemoryBarrierCount = 1;
	dependencyInfo.pBufferMemoryBarriers = &barrier;
	dependencyInfo.imageMemoryBarrierCount = 0;
	dependencyInfo.pImageMemoryBarriers = nullptr;

	vkCmdPipelineBarrier2(cmd, &dependencyInfo);
}
void ashen::Buffer::CopyToBuffer(VkCommandBuffer cmd, const Buffer& dst, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) const
{
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, m_Buffer, dst.GetHandle(), 1, &copyRegion);
}
void ashen::Buffer::CopyToImage(VkCommandBuffer cmd, const Image& dst, VkExtent3D extent) const
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { .x = 0, .y = 0, .z = 0 };
	region.imageExtent = extent;

	vkCmdCopyBufferToImage(cmd, m_Buffer, dst.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}
void ashen::Buffer::MapData(const void* pData, uint32_t size) const
{
	void* data = nullptr;
	vkMapMemory(m_pContext->GetDevice(), m_Memory, 0, VK_WHOLE_SIZE, 0, &data);
	memcpy(data, pData, size);
	vkUnmapMemory(m_pContext->GetDevice(), m_Memory);
}


//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//? ~~	  Buffer Allocator	
//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------

ashen::BufferAllocator::BufferAllocator(VulkanContext& context)
	: m_pContext(&context)
{
	m_CreateInfo = {};
	m_CreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	m_CreateInfo.size = 0;
	m_CreateInfo.usage = 0;
	m_CreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	m_UseInitialData = false;
}

//--------------------------------------------------
//    Allocator
//--------------------------------------------------
ashen::BufferAllocator& ashen::BufferAllocator::SetSize(uint32_t size)
{
	m_CreateInfo.size = size;
	return *this;
}
ashen::BufferAllocator& ashen::BufferAllocator::SetUsage(VkBufferUsageFlags usage)
{
	m_CreateInfo.usage = usage;
	return *this;
}
ashen::BufferAllocator& ashen::BufferAllocator::SetSharingMode(VkSharingMode sharingMode)
{
	m_CreateInfo.sharingMode = sharingMode;
	return *this;
}
ashen::BufferAllocator& ashen::BufferAllocator::HostAccess(bool access)
{
	m_Properties = access ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	return *this;
}
ashen::BufferAllocator& ashen::BufferAllocator::AddInitialData(void* data, VkDeviceSize dstOffset, uint32_t size)
{
	m_UseInitialData = true;

	m_pData = data;
	m_DstOffset = dstOffset;
	m_InitDataSize = size;

	m_CreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	return *this;
}

void ashen::BufferAllocator::Allocate(Buffer& buffer)
{
	buffer.m_pContext = m_pContext;

	if (vkCreateBuffer(m_pContext->GetDevice(), &m_CreateInfo, nullptr, &buffer.m_Buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Buffer!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_pContext->GetDevice(), buffer.m_Buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_pContext->GetPhysicalDevice(), &memProperties);
	uint32_t outcome = 0;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & m_Properties) == m_Properties)
		{
			outcome = i;
			break;
		}
	}
	allocInfo.memoryTypeIndex = outcome;

	if (vkAllocateMemory(m_pContext->GetDevice(), &allocInfo, nullptr, &buffer.m_Memory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");

	vkBindBufferMemory(m_pContext->GetDevice(), buffer.m_Buffer, buffer.m_Memory, 0);

	buffer.m_Size = m_CreateInfo.size;

	if (m_UseInitialData)
	{
		Buffer stagingBuffer;
		BufferAllocator stagingAllocator{ *m_pContext };
		stagingAllocator
			.SetUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.SetSize(m_InitDataSize)
			.HostAccess(true)
			.Allocate(stagingBuffer);

		void* data = nullptr;
		vkMapMemory(m_pContext->GetDevice(), stagingBuffer.m_Memory, 0, VK_WHOLE_SIZE, 0, &data);
		memcpy(data, m_pData, m_InitDataSize);
		vkUnmapMemory(m_pContext->GetDevice(), stagingBuffer.m_Memory);

		VkCommandBufferAllocateInfo allocInfoCmd{};
		allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfoCmd.commandPool = m_pContext->GetCommandPool();
		allocInfoCmd.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_pContext->GetDevice(), &allocInfoCmd, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		{
			stagingBuffer.CopyToBuffer(commandBuffer, buffer, m_InitDataSize, 0, m_DstOffset);
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
}
