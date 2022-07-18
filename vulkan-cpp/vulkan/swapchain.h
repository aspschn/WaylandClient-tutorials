#ifndef _SWAPCHAIN_H
#define _SWAPCHAIN_H

// C
#include <stdint.h>

// C++
#include <memory>
#include <vector>

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

    VkSurfaceFormatKHR surface_format() const;

    std::vector<VkImage> images() const;

    VkExtent2D extent() const;

private:
    VkSwapchainKHR _vk_swapchain;
    VkSurfaceFormatKHR _surface_format;
    VkPresentModeKHR _present_mode;
    uint32_t _image_count;
    VkExtent2D _extent;
    std::vector<VkImage> _images;
};

} // namespace vk

#endif // _SWAPCHAIN_H