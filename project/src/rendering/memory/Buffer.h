#ifndef ASHEN_BUFFER_H
#define ASHEN_BUFFER_H

// -- Standard Library --
#include <vector>

// -- Vulkan Includes --
#include <vulkan/vulkan.h>

// -- Forward Declares ))
namespace ashen
{
	class Image;
	class VulkanContext;
}

namespace ashen
{
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  Buffer	
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class Buffer final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		explicit Buffer() = default;
		~Buffer();

		Buffer(const Buffer& other) = delete;
		Buffer(Buffer&& other) noexcept = default;
		Buffer& operator=(const Buffer& other) = delete;
		Buffer& operator=(Buffer&& other) noexcept = default;

		//--------------------------------------------------
		//    Accessors & Mutators
		//--------------------------------------------------
		const VkBuffer& GetHandle() const;
		const VkDeviceMemory& GetMemoryHandle() const;
		VkDeviceSize Size() const;

		//--------------------------------------------------
		//    Commands
		//--------------------------------------------------
		void InsertBarrier(VkCommandBuffer cmd, VkAccessFlags2 srcAccess, VkPipelineStageFlags2 srcStage, VkAccessFlags2 dstAccess, VkPipelineStageFlags2 dstStage) const;
		void CopyToBuffer(VkCommandBuffer cmd, const Buffer& dst, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset) const;
		void CopyToImage(VkCommandBuffer cmd, const Image& dst, VkExtent3D extent) const;
		void MapData(const void* pData, uint32_t size) const;

	private:
		VkDeviceMemory m_Memory;
		VkBuffer m_Buffer;
		VkDeviceSize m_Size;

		VulkanContext* m_pContext;

		friend class BufferAllocator;
	};

	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~	  Buffer Allocator	
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	class BufferAllocator final
	{
	public:
		//--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
		BufferAllocator(VulkanContext& context);

		//--------------------------------------------------
		//    Allocator
		//--------------------------------------------------
		BufferAllocator& SetSize(uint32_t size);
		BufferAllocator& SetUsage(VkBufferUsageFlags usage);
		BufferAllocator& SetSharingMode(VkSharingMode sharingMode);
		BufferAllocator& HostAccess(bool access);
		BufferAllocator& AddInitialData(void* data, VkDeviceSize dstOffset, uint32_t size);

		void Allocate(Buffer& buffer);

	private:
		bool m_UseInitialData{};
		void* m_pData{};
		uint32_t m_InitDataSize{};
		VkDeviceSize m_DstOffset{};

		VulkanContext* m_pContext{};

		VkMemoryPropertyFlags m_Properties{};
		VkBufferCreateInfo m_CreateInfo{};
	};
}

#endif // ASHEN_BUFFER_H
