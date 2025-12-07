#ifndef ASHEN_VERTEX_H
#define ASHEN_VERTEX_H

// -- Standard Library --
#include <array>

// -- Math Includes --
#include "glm/glm.hpp"

// -- Vulkan Includes --
#include "vulkan/vulkan.h"

namespace ashen
{
    //? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//? ~~    Vertex
	//? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(Vertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return binding;
        }

        static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 2> attrs{};
            attrs[0].binding = 0;
            attrs[0].location = 0;
            attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[0].offset = offsetof(Vertex, pos);

            attrs[1].binding = 0;
            attrs[1].location = 1;
            attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[1].offset = offsetof(Vertex, color);

            return attrs;
        }
    };
}

#endif // ASHEN_VERTEX_H