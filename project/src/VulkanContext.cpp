#include "VulkanContext.h"
#include <iostream>

VulkanContext::VulkanContext(Window* window)
{
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Ashen")
        .request_validation_layers(true)
        .use_default_debug_messenger()
		.require_api_version(VK_API_VERSION_1_3)
        .build();
    if (!inst_ret) throw std::runtime_error("Failed to create Vulkan instance");
    m_VkbInstance = inst_ret.value();

    if (glfwCreateWindowSurface(m_VkbInstance.instance, window->GetGLFWwindow(), nullptr, &m_Surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create Vulkan surface");

	// -- Features --
	// -- Vulkan API Core Features --
	VkPhysicalDeviceFeatures vulkanCoreFeatures{};
	vulkanCoreFeatures.samplerAnisotropy = VK_TRUE;
	vulkanCoreFeatures.fillModeNonSolid = VK_TRUE;
	vulkanCoreFeatures.sampleRateShading = VK_TRUE;

	// -- Vulkan API 1.1 Features --
	VkPhysicalDeviceVulkan11Features vulkan11Features{};
	vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	// -- Vulkan API 1.2 Features --
	VkPhysicalDeviceVulkan12Features vulkan12Features{};
	vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12Features.runtimeDescriptorArray = VK_TRUE;
	vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
	vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
	vulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	vulkan12Features.descriptorIndexing = VK_TRUE;
	vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

	// -- Vulkan API 1.3 Features --
	VkPhysicalDeviceVulkan13Features vulkan13Features{};
	vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13Features.dynamicRendering = VK_TRUE;
	vulkan13Features.synchronization2 = VK_TRUE;

    vkb::PhysicalDeviceSelector selector{ m_VkbInstance };
    auto phys_ret = selector
		.set_surface(m_Surface)
		.add_required_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
		.add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
		.add_required_extension(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME)
		.add_required_extension(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME)
		.set_required_features(vulkanCoreFeatures)
		.set_required_features_11(vulkan11Features)
		.set_required_features_12(vulkan12Features)
		.set_required_features_13(vulkan13Features)
		.select();
    if (!phys_ret) throw std::runtime_error("Failed to select GPU");
    m_VkbPhysicalDevice = phys_ret.value();

    vkb::DeviceBuilder device_builder{ m_VkbPhysicalDevice };
    auto dev_ret = device_builder
		.build();
    if (!dev_ret) throw std::runtime_error("Failed to create device");
    m_VkbDevice = dev_ret.value();

    int w;
    int h;
    glfwGetFramebufferSize(window->GetGLFWwindow(), &w, &h);
    auto swap_ret = vkb::SwapchainBuilder(m_VkbDevice, m_Surface)
        .use_default_format_selection()
        .set_desired_min_image_count(2)
        .set_desired_extent(w, h)
        .build();
    if (!swap_ret) throw std::runtime_error("Failed to create swapchain");
    m_VkbSwapchain = swap_ret.value();
}
VulkanContext::~VulkanContext()
{
	vkDeviceWaitIdle(m_VkbDevice.device);
    vkb::destroy_swapchain(m_VkbSwapchain);
    vkb::destroy_device(m_VkbDevice);
    vkb::destroy_surface(m_VkbInstance, m_Surface);
    vkb::destroy_instance(m_VkbInstance);
}

VkInstance VulkanContext::GetInstance()                     const   { return m_VkbInstance.instance; }
VkDevice VulkanContext::GetDevice()                         const   { return m_VkbDevice.device; }
VkPhysicalDevice VulkanContext::GetPhysicalDevice()         const   { return m_VkbPhysicalDevice.physical_device; }

VkQueue VulkanContext::GetQueue(vkb::QueueType type)        const   { return m_VkbDevice.get_queue(type).value(); }
uint32_t VulkanContext::GetQueueIndex(vkb::QueueType type)  const   { return m_VkbDevice.get_queue_index(type).value(); }

VkSwapchainKHR VulkanContext::GetSwapchain()                const   { return m_VkbSwapchain.swapchain; }
uint32_t VulkanContext::GetSwapchainImageCount()            const   { return m_VkbSwapchain.image_count; }
std::vector<VkImage> VulkanContext::GetSwapchainImages()            { return m_VkbSwapchain.get_images().value(); }
std::vector<VkImageView> VulkanContext::GetSwapchainImageViews()    { return m_VkbSwapchain.get_image_views().value(); }
VkExtent2D VulkanContext::GetSwapchainExtent()              const   { return m_VkbSwapchain.extent; }
VkFormat VulkanContext::GetSwapchainFormat()                const   { return m_VkbSwapchain.image_format; }

VkSurfaceKHR VulkanContext::GetSurface() const { return m_Surface; }
