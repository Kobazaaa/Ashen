#include "VulkanContext.h"
#include <iostream>

VulkanContext::VulkanContext(Window* window)
{
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Ashen")
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .build();
    if (!inst_ret) throw std::runtime_error("Failed to create Vulkan instance");
    m_VkbInstance = inst_ret.value();

    if (glfwCreateWindowSurface(m_VkbInstance.instance, window->GetGLFWwindow(), nullptr, &m_Surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Vulkan surface");

    vkb::PhysicalDeviceSelector selector{ m_VkbInstance };
    auto phys_ret = selector.set_surface(m_Surface).select();
    if (!phys_ret) throw std::runtime_error("Failed to select GPU");
    m_VkbPhysicalDevice = phys_ret.value();

    vkb::DeviceBuilder device_builder{ m_VkbPhysicalDevice };
    auto dev_ret = device_builder.build();
    if (!dev_ret) throw std::runtime_error("Failed to create device");
    m_VkbDevice = dev_ret.value();
}
VulkanContext::~VulkanContext()
{
    vkb::destroy_device(m_VkbDevice);
    vkb::destroy_surface(m_VkbInstance, m_Surface);
    vkb::destroy_instance(m_VkbInstance);
}

VkInstance VulkanContext::GetInstance() const
{
	return m_VkbInstance.instance;
}
VkDevice VulkanContext::GetDevice() const
{
	return m_VkbDevice.device;
}
VkPhysicalDevice VulkanContext::GetPhysicalDevice() const
{
	return m_VkbPhysicalDevice.physical_device;
}
VkSurfaceKHR VulkanContext::GetSurface() const
{
	return m_Surface;
}
