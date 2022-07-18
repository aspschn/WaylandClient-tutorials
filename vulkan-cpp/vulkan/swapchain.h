#ifndef _SWAPCHAIN_H
#define _SWAPCHAIN_H

// C
#include <stdint.h>

// C++
#include <memory>

// Vulkan
#include <vulkan/vulkan.h>

namespace vk {

class Instance;
class Surface;
class Device;

class Swapchain
{
public:
    Swapchain(std::shared_ptr<Instance> instance,
            std::shared_ptr<Surface> surface,
            std::shared_ptr<Device> device,
            uint32_t width, uint32_t height);

    VkSwapchainKHR vk_swapchain();

private:
    VkSwapchainKHR _vk_swapchain;
    VkSurfaceFormatKHR _surface_format;
    VkPresentModeKHR _present_mode;
    uint32_t _image_count;
    VkExtent2D _extent;
};

} // namespace vk

#endif // _SWAPCHAIN_H
