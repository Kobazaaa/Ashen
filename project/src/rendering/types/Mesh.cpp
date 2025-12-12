// -- Ashen Includes --
#include "Mesh.h"


//--------------------------------------------------
//    Constructor & Destructor
//--------------------------------------------------
ashen::Mesh::Mesh(VulkanContext& context, const std::vector<Vertex>& v, const std::vector<uint32_t>& i)
	: m_vVertices(v)
	, m_vIndices(i)
	, m_pContext(&context)
{
    uint32_t vBufferSize = sizeof(m_vVertices[0]) * static_cast<uint32_t>(m_vVertices.size());
	BufferAllocator bufferAlloc{ context };
    bufferAlloc
        .SetSize(vBufferSize)
        .HostAccess(false)
        .SetUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        .AddInitialData(m_vVertices.data(), 0, vBufferSize)
		.Allocate(m_VertexBuffer);

    uint32_t iBufferSize = sizeof(m_vIndices[0]) * static_cast<uint32_t>(m_vIndices.size());
    bufferAlloc = { context };
    bufferAlloc
        .SetSize(iBufferSize)
        .HostAccess(false)
        .SetUsage(VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        .AddInitialData((void*)m_vIndices.data(), 0, iBufferSize)
		.Allocate(m_IndexBuffer);
}
ashen::Mesh::~Mesh()
{ }


//--------------------------------------------------
//    Functionality
//--------------------------------------------------
void ashen::Mesh::Bind(VkCommandBuffer cmd) const
{
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_VertexBuffer.GetHandle(), offsets);
    vkCmdBindIndexBuffer(cmd, m_IndexBuffer.GetHandle(), 0, VK_INDEX_TYPE_UINT32);
}
void ashen::Mesh::Draw(VkCommandBuffer cmd) const
{
    vkCmdDrawIndexed(cmd, static_cast<uint32_t>(m_vIndices.size()), 1, 0, 0, 0);
}


//--------------------------------------------------
//    Accessors & Mutators
//--------------------------------------------------
