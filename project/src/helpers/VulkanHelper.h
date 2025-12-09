#ifndef ASHEN_VULKAN_HELPER_H
#define ASHEN_VULKAN_HELPER_H

// -- Standard Library --
#include <stdexcept>

// -- Ashen Includes --
#include "VulkanContext.h"

namespace ashen
{
    static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, const VulkanContext* pContext)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(pContext->GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(pContext->GetDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(pContext->GetPhysicalDevice(), &memProperties);
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

        if (vkAllocateMemory(pContext->GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate buffer memory");

        vkBindBufferMemory(pContext->GetDevice(), buffer, bufferMemory, 0);
    }
    static void UploadToBuffer(const VkDeviceMemory& bufferMemory, void* data, VkDeviceSize size, const VulkanContext* pContext)
    {
        void* pData;
        vkMapMemory(pContext->GetDevice(), bufferMemory, 0, VK_WHOLE_SIZE, 0, &pData);
        memcpy(pData, data, size);
        vkUnmapMemory(pContext->GetDevice(), bufferMemory);
    }

}

#endif // ASHEN_VULKAN_HELPER_H
