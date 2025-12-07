#ifndef ASHEN_RENDERER_H
#define ASHEN_RENDERER_H

#include "Vertex.h"
#include "VulkanContext.h"
#include "Window.h"

class Renderer final
{
public:
    Renderer(VulkanContext* pContext, Window* pWindow);
    ~Renderer();
    Renderer(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;

    void Render();

private:
    void CreateBuffers();
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    void UploadToBuffer(const VkDeviceMemory& bufferMemory, void* data, VkDeviceSize size) const;
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CreatePipeline();
    void LoadShaderModule(const std::string& filename, VkShaderModule& shaderMod) const;

	VulkanContext* m_pContext;
    Window* m_pWindow;

    // Buffers
    std::vector<VkCommandBuffer> m_vCommandBuffers;
    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;

    // Pipeline
    VkPipelineLayout m_PipelineLayout{};
    VkPipeline m_Pipeline{};

    // Sync
    std::vector<VkSemaphore> m_vImageAvailableSemaphores;
    std::vector<VkSemaphore> m_vRenderFinishedSemaphores;
    std::vector<VkFence> m_vInFlightFences;
    size_t m_CurrentFrame = 0;

    // Vertices
    const std::vector<Vertex> m_vVertices = {
    {.pos = {0.0f, -0.5f, 1.0f}, .color = {1.0f, 0.0f, 1.0f}},
    {.pos = {0.5f, 0.5f, 1.0f}, .color = {0.0f, 1.0f, 1.0f}},
    {.pos = {-0.5f, 0.5f, 1.0f}, .color = {0.0f, 0.0f, 1.0f}}
    };
    const std::vector<uint16_t> m_vIndices = { 0, 1, 2 };
};

#endif // ASHEN_WINDOW_H