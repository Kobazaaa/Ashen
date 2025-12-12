#ifndef ASHEN_MESH_H
#define ASHEN_MESH_H

// -- Standard Library --
#include <vector>

// -- Ashen Includes --
#include "Buffer.h"
#include "Vertex.h"
#include "VulkanContext.h"

namespace ashen
{
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //? ~~    Mesh
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class Mesh final
    {
    public:
        //--------------------------------------------------
        //    Constructor & Destructor
        //--------------------------------------------------
        explicit Mesh(VulkanContext& context, const std::vector<Vertex>& v, const std::vector<uint32_t>& i);
        ~Mesh();

        Mesh(const Mesh& other) = delete;
        Mesh(Mesh&& other) noexcept = delete;
        Mesh& operator=(const Mesh& other) = delete;
        Mesh& operator=(Mesh&& other) noexcept = delete;

        //--------------------------------------------------
        //    Functionality
        //--------------------------------------------------
        void Bind(VkCommandBuffer cmd) const;
        void Draw(VkCommandBuffer cmd) const;

        //--------------------------------------------------
        //    Accessors & Mutators
        //--------------------------------------------------

    private:
        Buffer m_VertexBuffer{};
        Buffer m_IndexBuffer{};

        std::vector<Vertex> m_vVertices{};
        std::vector<uint32_t> m_vIndices{};

        VulkanContext* m_pContext;
    };
}

#endif // ASHEN_MESH_H
