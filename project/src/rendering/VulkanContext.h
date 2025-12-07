#ifndef ASHEN_CONTEXT_H
#define ASHEN_CONTEXT_H

// -- Vulkan Includes --
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

// -- Ashen Includes --
#include "Window.h"

namespace ashen
{
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~    VulkanContext
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class VulkanContext final
    {
    public:
        //--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
        VulkanContext(Window* window);
        ~VulkanContext();

        VulkanContext(const VulkanContext& other) = delete;
        VulkanContext(VulkanContext&& other) = delete;
        VulkanContext& operator=(const VulkanContext& other) = delete;
        VulkanContext& operator=(VulkanContext&& other) = delete;

        //--------------------------------------------------
		//    Base Objects
		//--------------------------------------------------
        VkInstance GetInstance() const;
        VkDevice GetDevice() const;
        VkPhysicalDevice GetPhysicalDevice() const;

        //--------------------------------------------------
		//    Queue Objects
		//--------------------------------------------------
        VkQueue GetQueue(vkb::QueueType type) const;
        uint32_t GetQueueIndex(vkb::QueueType type) const;

        //--------------------------------------------------
		//    Swapchain Objects
		//--------------------------------------------------
        void RebuildSwapchain(glm::uvec2 size);
        VkSwapchainKHR GetSwapchain() const;
        uint32_t GetSwapchainImageCount() const;
        std::vector<VkImage> GetSwapchainImages();
        std::vector<VkImageView> GetSwapchainImageViews();
        VkExtent2D GetSwapchainExtent() const;
        VkFormat GetSwapchainFormat() const;

        //--------------------------------------------------
		//    Other Objects
		//--------------------------------------------------
        VkSurfaceKHR GetSurface() const;

    private:
        vkb::Instance m_VkbInstance;
        vkb::Device m_VkbDevice;
        vkb::PhysicalDevice m_VkbPhysicalDevice;
        vkb::Swapchain m_VkbSwapchain;
        VkSurfaceKHR m_Surface{};
        std::vector<VkImageView> m_vSwapchainImageViews{};
    };
}

#endif // ASHEN_CONTEXT_H