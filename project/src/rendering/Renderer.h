#ifndef ASHEN_RENDERER_H
#define ASHEN_RENDERER_H

// -- Standard Library --
#include <memory>

// -- Ashen Includes --
#include "Camera.h"
#include "Mesh.h"
#include "Pipeline.h"
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
        Renderer(Window* pWindow);
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
        void CreateDepthResources(VkExtent2D extent);
        void OnResize();
        void RecordCommandBuffer(uint32_t imageIndex);
        void CreateCommandBuffers();
        void CreateSyncObjects();

        std::unique_ptr<VulkanContext> m_pContext;
        Window* m_pWindow;
        std::unique_ptr<Camera> m_pCamera;

        // Buffers
        std::vector<VkCommandBuffer> m_vCommandBuffers;
        std::vector<Image> m_vDepthImages;

        // Pipeline
        Pipeline m_PipelineDefault{};
        Pipeline m_PipelineSky{};

        // Sync
        std::vector<VkSemaphore> m_vImageAvailableSemaphores;
        std::vector<VkSemaphore> m_vRenderFinishedSemaphores;
        std::vector<VkFence> m_vInFlightFences;
        size_t m_CurrentFrame = 0;

        // Vertices
        std::unique_ptr<Mesh> m_pMeshFloor;
        std::unique_ptr<Mesh> m_pMeshSky;
    };

}

#endif // ASHEN_WINDOW_H