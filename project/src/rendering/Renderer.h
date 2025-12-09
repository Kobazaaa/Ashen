#ifndef ASHEN_RENDERER_H
#define ASHEN_RENDERER_H

// -- Standard Library --
#include <memory>

// -- Ashen Includes --
#include "Camera.h"
#include "Pipeline.h"
#include "Vertex.h"
#include "VulkanContext.h"
#include "Window.h"

namespace ashen
{
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~    Renderer
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class Renderer final
    {
    public:
        //--------------------------------------------------
		//    Constructor & Destructor
		//--------------------------------------------------
        Renderer(VulkanContext* pContext, Window* pWindow);
        ~Renderer();

        Renderer(const Renderer& other) = delete;
        Renderer(Renderer&& other) = delete;
        Renderer& operator=(const Renderer& other) = delete;
        Renderer& operator=(Renderer&& other) = delete;

        //--------------------------------------------------
		//    Functionality
		//--------------------------------------------------
        void Update();
        void Render();

    private:
        void OnResize() const;
        void RecordCommandBuffer(uint32_t imageIndex);
        void CreateBuffers();
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
        void UploadToBuffer(const VkDeviceMemory& bufferMemory, void* data, VkDeviceSize size) const;
        void CreateCommandBuffers();
        void CreateSyncObjects();

        VulkanContext* m_pContext;
        Window* m_pWindow;
        std::unique_ptr<Camera> m_pCamera;

        // Buffers
        VkCommandPool m_CommandPool{};
        std::vector<VkCommandBuffer> m_vCommandBuffers;
        VkBuffer m_VertexBuffer;
        VkDeviceMemory m_VertexBufferMemory;
        VkBuffer m_IndexBuffer;
        VkDeviceMemory m_IndexBufferMemory;

        // Pipeline
        std::unique_ptr<Pipeline> m_pPipeline{};

        // Sync
        std::vector<VkSemaphore> m_vImageAvailableSemaphores;
        std::vector<VkSemaphore> m_vRenderFinishedSemaphores;
        std::vector<VkFence> m_vInFlightFences;
        size_t m_CurrentFrame = 0;

        // Vertices
#define VERTEX_COLOR_CHANNEL 0.5f
#define VERTEX_COLOR {VERTEX_COLOR_CHANNEL, VERTEX_COLOR_CHANNEL, VERTEX_COLOR_CHANNEL}
        const std::vector<Vertex> m_vVertices = {
	        {.pos = { 1000.f, -1.f,  1000.f}, .color = VERTEX_COLOR},
	        {.pos = { 1000.f, -1.f, -1000.f}, .color = VERTEX_COLOR},
	        {.pos = {-1000.f, -1.f, -1000.f}, .color = VERTEX_COLOR},
	        {.pos = {-1000.f, -1.f,  1000.f}, .color = VERTEX_COLOR}
        };
#undef VERTEX_COLOR
#undef VERTEX_COLOR_CHANNEL
        const std::vector<uint16_t> m_vIndices = { 0, 1, 2, 0, 2, 3 };
    };

}

#endif // ASHEN_WINDOW_H