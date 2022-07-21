#ifndef _VERTEX_H
#define _VERTEX_H

#include <array>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace vk {

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

public:
    static VkVertexInputBindingDescription
    get_binding_description()
    {
        VkVertexInputBindingDescription desc;
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 2>
    get_attribute_descriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> arr;

        arr[0].binding = 0;
        arr[0].location = 0;
        arr[0].format = VK_FORMAT_R32G32_SFLOAT;
        arr[0].offset = offsetof(Vertex, pos);

        arr[1].binding = 0;
        arr[1].location = 1;
        arr[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        arr[1].offset = offsetof(Vertex, color);

        return arr;
    }
};

} // namespace vk

#endif // _VERTEX_H
