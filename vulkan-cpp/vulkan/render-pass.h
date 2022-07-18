#ifndef _RENDER_PASS_H
#define _RENDER_PASS_H

// C++
#include <memory>

// Vulkan
#include <vulkan/vulkan.h>

namespace vk {

class Device;

class RenderPass
{
public:
    RenderPass(VkFormat format,
            std::shared_ptr<Device> device);

    VkRenderPass vk_render_pass() const;

private:
    VkRenderPass _vk_render_pass;
};

} // namespace vk

#endif // _RENDER_PASS_H
