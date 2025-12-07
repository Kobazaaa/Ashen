#ifndef ASHEN_CONTEXT_H
#define ASHEN_CONTEXT_H

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include "Window.h"

class VulkanContext final
{
public:
    VulkanContext(Window* window);
    ~VulkanContext();
    VulkanContext(const VulkanContext& other) = delete;
    VulkanContext(VulkanContext&& other) = delete;
    VulkanContext& operator=(const VulkanContext& other) = delete;
    VulkanContext& operator=(VulkanContext&& other) = delete;

    VkInstance GetInstance() const;
    VkDevice GetDevice() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkSurfaceKHR GetSurface() const;

private:
    vkb::Instance m_VkbInstance;
    vkb::Device m_VkbDevice;
    vkb::PhysicalDevice m_VkbPhysicalDevice;
    VkSurfaceKHR m_Surface{};
};

#endif // ASHEN_CONTEXT_H