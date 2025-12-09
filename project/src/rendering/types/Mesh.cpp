// -- Ashen Includes --
#include "Mesh.h"
#include "VulkanHelper.h"


//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Mesh::Mesh(VulkanContext& context, const std::vector<Vertex>& v, const std::vector<uint32_t>& i)
	: m_vVertices(v)
	, m_vIndices(i)
	, m_pContext(&context)
{
    auto vBufferSize = sizeof(m_vVertices[0]) * m_vVertices.size();
    CreateBuffer(vBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_VertexBuffer, m_VertexBufferMemory, m_pContext);

    auto iBufferSize = sizeof(m_vIndices[0]) * m_vIndices.size();
    CreateBuffer(iBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_IndexBuffer, m_IndexBufferMemory, m_pContext);

    UploadToBuffer(m_VertexBufferMemory, (void*)m_vVertices.data(), vBufferSize, m_pContext);
    UploadToBuffer(m_IndexBufferMemory, (void*)m_vIndices.data(), vBufferSize, m_pContext);
}
ashen::Mesh::~Mesh()
{
    auto device = m_pContext->GetDevice();
    vkDestroyBuffer(device, m_VertexBuffer, nullptr);
    vkFreeMemory(device, m_VertexBufferMemory, nullptr);
    vkDestroyBuffer(device, m_IndexBuffer, nullptr);
    vkFreeMemory(device, m_IndexBufferMemory, nullptr);
}


//--------------------------------------------------
//    Functionality
//--------------------------------------------------
void ashen::Mesh::Bind(VkCommandBuffer cmd) const
{
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
}
void ashen::Mesh::Draw(VkCommandBuffer cmd) const
{
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_vIndices.size()), 1, 0, 0, 0);
}


//--------------------------------------------------
//    Accessors & Mutators
//--------------------------------------------------
