#ifndef ASHEN_RENDERER_H
#define ASHEN_RENDERER_H

// -- Standard Library --
#include <memory>
#include <numbers>

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
		// -- Planet --
        float m_PlanetRadius                        { 6'371'000.f };
        float m_AtmosphereThickness                 { 100'000 };
        float m_RenderPlanetRadius                  { 20.f };
        float m_RenderAtmosphereThickness           { m_RenderPlanetRadius / m_PlanetRadius * m_AtmosphereThickness };

        // -- Settings --
        int m_SampleCount                           { 16 };
        float m_ESun                                { 20.f };                           // Strength of the Sun
        float m_g                                   { -0.990f };                        // Scattering constant g that affects symmetry

		// -- Rayleigh --
        glm::vec3 m_BetaRayleigh                    { /* set in constructor */ };
        float m_RayleighScaleDepth                  { 7994.f };

        // -- Mie --
        glm::vec3 m_BetaMie	                        { /* set in constructor */ };
        float m_MieScaleDepth                       { 1200.f };

        // -- Light --
        glm::vec3 m_LightDirection                  { };
        std::vector<glm::vec3> m_vLightDirections   { };
        uint32_t m_LightIndex                       { 0u };

        // -- Extra --
        bool m_UseHDR                               { true };
        float m_Exposure                            { 2.0f };

        // -- Meshes --
        std::unique_ptr<Mesh>   m_pMeshFloor;
        std::unique_ptr<Mesh>   m_pMeshSky;
        std::unique_ptr<Camera> m_pCamera;

        // -- Pipelines --
        Pipeline                        m_SkyFromAtmosphere     { };
        std::vector<DescriptorSet>      m_vDescriptorSetsSky    { };
        UniformBufferGroup<SkyVS>       m_vUBOSky_VS            { };
        UniformBufferGroup<SkyFS>       m_vUBOSky_FS            { };

        Pipeline                        m_GroundFromAtmosphere  { };
        std::vector<DescriptorSet>      m_vDescriptorSetsGround { };
        UniformBufferGroup<GroundVS>    m_vUBOGround_VS         { };

		//--------------------------------------------------
        //    Rendering
        //--------------------------------------------------

        // -- Meshes --
        std::unique_ptr<Mesh> CreateDome(float radius, int segmentsLat, int segmentsLon) const;

        // -- Creation --
        void CreateSamplers();
        void CreatePipelines(VkFormat renderFormat);
        void CreateDescriptorSets();
        void CreateDepthResources(VkExtent2D extent);
        void CreateRenderTargets(VkExtent2D extent);
        void CreateCommandBuffers();
        void CreateSyncObjects();

        // -- Frame --
        void SetupFrame(uint32_t imageIndex) const;
        void SetRenderTarget(VkImageView view, VkImageLayout layout);
        void EndRenderTarget() const;
        void RenderFrame(uint32_t imageIndex);
        void EndFrame(uint32_t imageIndex) const;
        void RecordCommandBuffer(uint32_t imageIndex);
        void OnResize();

        // -- Buffers --
        DescriptorPool m_DescriptorPool{};
        std::vector<VkCommandBuffer> m_vCommandBuffers;
        std::vector<Image> m_vDepthImages;

        std::vector<Image>              m_vRenderTargets;
        Pipeline                        m_PostProcess{ };
        std::vector<DescriptorSet>      m_vDescriptorSetsPostProcess{ };
        VkSampler                       m_PostProcessSampler{};

        // -- Sync --
        std::vector<VkSemaphore> m_vImageAvailableSemaphores;
        std::vector<VkSemaphore> m_vRenderFinishedSemaphores;
        std::vector<VkFence> m_vInFlightFences;
        uint32_t m_CurrentFrame = 0;

        // -- Helper --
        void HandleInput();
        void PrintStats();
    };

}

#endif // ASHEN_WINDOW_H