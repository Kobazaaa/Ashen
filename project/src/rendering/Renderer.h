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
        // -- Context --
        Window* m_pWindow;
        std::unique_ptr<VulkanContext> m_pContext;

        //--------------------------------------------------
		//    Scattering
		//--------------------------------------------------
        // -- Settings --
        float m_InnerRadius     { 10.f };
        float m_OuterRadius     { 10.25f };
        float m_Scale           { 1.f / (m_OuterRadius - m_InnerRadius) };

        // -- Meshes --
        std::unique_ptr<Mesh>   m_pMeshFloor;
        std::unique_ptr<Mesh>   m_pMeshSky;
        std::unique_ptr<Camera> m_pCamera;

        // -- Pipelines --
        Pipeline                        m_SkyFromSpace          { };
        Pipeline                        m_SkyFromAtmosphere     { };
        std::vector<DescriptorSet>      m_vDescriptorSetsSky    { };
        UniformBufferGroup<SkyVS>       m_vUBOSky_VS            { };
        UniformBufferGroup<SkyFS>       m_vUBOSky_FS            { };

        Pipeline                        m_GroundFromSpace       { };
        Pipeline                        m_GroundFromAtmosphere  { };
        std::vector<DescriptorSet>      m_vDescriptorSetsGround { };
        UniformBufferGroup<GroundVS>    m_vUBOGround_VS         { };
        UniformBufferGroup<GroundFS>    m_vUBOGround_FS         { };

        Pipeline                        m_SpaceFromSpace        { };
        Pipeline                        m_SpaceFromAtmosphere   { };
        std::vector<DescriptorSet>      m_vDescriptorSetsSpace  { };
        UniformBufferGroup<SpaceVS>     m_vUBOSpace_VS          { };
        UniformBufferGroup<SpaceFS>     m_vUBOSpace_FS          { };



		//--------------------------------------------------
        //    Rendering
        //--------------------------------------------------

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

        // -- Buffers --
        DescriptorPool m_DescriptorPool{};
        std::vector<VkCommandBuffer> m_vCommandBuffers;
        std::vector<Image> m_vDepthImages;

        // -- Sync --
        std::vector<VkSemaphore> m_vImageAvailableSemaphores;
        std::vector<VkSemaphore> m_vRenderFinishedSemaphores;
        std::vector<VkFence> m_vInFlightFences;
        uint32_t m_CurrentFrame = 0;
    };

}

#endif // ASHEN_WINDOW_H