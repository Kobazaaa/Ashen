// -- Standard Library --
#include <iostream>

// -- Ashen Includes --
#include "VulkanContext.h"

//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::VulkanContext::VulkanContext(Window* window)
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

	VkSurfaceFormatKHR format{ VK_FORMAT_B8G8R8A8_SRGB , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	auto size = window->GetFramebufferSize();
    auto swap_ret = vkb::SwapchainBuilder(m_VkbDevice, m_Surface)
        .set_desired_format(format)
        .set_desired_min_image_count(2)
        .set_desired_extent(size.x, size.y)
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
        .build();
    if (!swap_ret) throw std::runtime_error("Failed to create swapchain");
    m_VkbSwapchain = swap_ret.value();
	m_vSwapchainImageViews = m_VkbSwapchain.get_image_views().value();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = GetQueueIndex(vkb::QueueType::graphics);

	vkCreateCommandPool(m_VkbDevice.device, &poolInfo, nullptr, &m_CommandPool);
}
ashen::VulkanContext::~VulkanContext()
{
	vkDeviceWaitIdle(m_VkbDevice.device);
	vkDestroyCommandPool(m_VkbDevice.device, m_CommandPool, nullptr);
	m_VkbSwapchain.destroy_image_views(m_vSwapchainImageViews);
    vkb::destroy_swapchain(m_VkbSwapchain);
    vkb::destroy_device(m_VkbDevice);
    vkb::destroy_surface(m_VkbInstance, m_Surface);
    vkb::destroy_instance(m_VkbInstance);
}

//--------------------------------------------------
//    Base Objects
//--------------------------------------------------
VkInstance ashen::VulkanContext::GetInstance()                     const   { return m_VkbInstance.instance; }
VkDevice ashen::VulkanContext::GetDevice()                         const   { return m_VkbDevice.device; }
VkPhysicalDevice ashen::VulkanContext::GetPhysicalDevice()         const   { return m_VkbPhysicalDevice.physical_device; }
VkCommandPool ashen::VulkanContext::GetCommandPool()			   const   { return m_CommandPool; }

//--------------------------------------------------
//    Queue Objects
//--------------------------------------------------
VkQueue ashen::VulkanContext::GetQueue(vkb::QueueType type)        const   { return m_VkbDevice.get_queue(type).value(); }
uint32_t ashen::VulkanContext::GetQueueIndex(vkb::QueueType type)  const   { return m_VkbDevice.get_queue_index(type).value(); }

//--------------------------------------------------
//    Swapchain Objects
//--------------------------------------------------
void ashen::VulkanContext::RebuildSwapchain(glm::uvec2 size)
{
	vkDeviceWaitIdle(m_VkbDevice.device);

	m_VkbSwapchain.destroy_image_views(m_vSwapchainImageViews);
	m_vSwapchainImageViews.clear();
	auto swap_ret = vkb::SwapchainBuilder(m_VkbDevice, m_Surface)
		.set_old_swapchain(m_VkbSwapchain)
		.set_desired_extent(size.x, size.y)
		.build();
	if (!swap_ret) throw std::runtime_error("Failed to create swapchain");
	vkb::destroy_swapchain(m_VkbSwapchain);
	m_VkbSwapchain = swap_ret.value();
	m_vSwapchainImageViews = m_VkbSwapchain.get_image_views().value();
}
VkSwapchainKHR ashen::VulkanContext::GetSwapchain()                const   { return m_VkbSwapchain.swapchain; }
uint32_t ashen::VulkanContext::GetSwapchainImageCount()            const   { return m_VkbSwapchain.image_count; }
std::vector<VkImage> ashen::VulkanContext::GetSwapchainImages()            { return m_VkbSwapchain.get_images().value(); }
std::vector<VkImageView> ashen::VulkanContext::GetSwapchainImageViews()    { return m_vSwapchainImageViews; }
VkExtent2D ashen::VulkanContext::GetSwapchainExtent()              const   { return m_VkbSwapchain.extent; }
VkFormat ashen::VulkanContext::GetSwapchainFormat()                const   { return m_VkbSwapchain.image_format; }

//--------------------------------------------------
//    Other Objects
//--------------------------------------------------
VkSurfaceKHR ashen::VulkanContext::GetSurface() const { return m_Surface; }
