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
        // -- Settings --
        int m_SampleCount           { 16 };

        float m_Kr                  { 0.0025f };        // Scattering constant for Rayleigh scattering
        float m_Km                  { 0.0010f };        // Scattering constant for Mie scattering
    	
        float m_Kr4PI               { m_Kr * 4.0f * std::numbers::pi_v<float> };
    	float m_Km4PI               { m_Km * 4.0f * std::numbers::pi_v<float> };
        
        float m_ESun                { 20.f };           // Strength of the Sun
        float m_g                   { -0.990f };        // Scattering constant g that affects symmetry


        float m_InnerRadius         { 10.f };
        float m_OuterRadius         { 10.25f };
        float m_Scale               { 1.f / (m_OuterRadius - m_InnerRadius) };

        glm::vec3 m_Light           { 0.f, 0.f, 1000.f };
        glm::vec3 m_LightDirection  { glm::normalize(m_Light) };

        glm::vec3 m_Wavelength	    { 0.650f, 0.570f, 0.475f }; // Wavelengths for RGB in order in nm
        glm::vec3 m_Wavelength4     { powf(m_Wavelength.x, 4), powf(m_Wavelength.y, 4) , powf(m_Wavelength.z, 4) };

        float m_RayleighScaleDepth  { 0.25f };
        float m_MieScaleDepth       { 0.1f };

        float m_Exposure            { 2.0f };
        bool m_UseHDR          { true };

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