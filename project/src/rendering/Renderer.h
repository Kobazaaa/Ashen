#ifndef ASHEN_RENDERER_H
#define ASHEN_RENDERER_H

// -- Standard Library --
#include <memory>

// -- Ashen Includes --
#include "Camera.h"
#include "Descriptors.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "Types.h"
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
        explicit Renderer(Window* pWindow);
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
        // -- Meshes --
        void CreatePlaneMesh();
        void CreateSkyMesh();

        // -- Creation --
        void CreatePipelines();
        void CreateDescriptorSets();
        void CreateDepthResources(VkExtent2D extent);
        void CreateCommandBuffers();
        void CreateSyncObjects();

        // -- Frame --
        void SetupFrame(uint32_t imageIndex) const;
        void RenderFrame();
        void EndFrame(uint32_t imageIndex) const;
        void RecordCommandBuffer(uint32_t imageIndex);
        void OnResize();

        std::unique_ptr<VulkanContext> m_pContext;
        Window* m_pWindow;
        std::unique_ptr<Camera> m_pCamera;

        // -- Buffers --
        DescriptorPool m_DescriptorPool{};
        std::vector<DescriptorSet> m_vDescriptorSets;

        UniformBufferGroup<SkyFromSpaceVS> m_vUBO_SFS_VS{};

        std::vector<VkCommandBuffer> m_vCommandBuffers;
        std::vector<Image> m_vDepthImages;

        // -- Rendering --
        Pipeline m_PipelineDefault{};
        Pipeline m_PipelineSky{};

        // -- Sync --
        std::vector<VkSemaphore> m_vImageAvailableSemaphores;
        std::vector<VkSemaphore> m_vRenderFinishedSemaphores;
        std::vector<VkFence> m_vInFlightFences;
        uint32_t m_CurrentFrame = 0;

        // -- Meshes --
        std::unique_ptr<Mesh> m_pMeshFloor;
        std::unique_ptr<Mesh> m_pMeshSky;
    };

}

#endif // ASHEN_WINDOW_H