#ifndef _SURFACE_H
#define _SURFACE_H

// C++
#include <memory>

// Wayland
#include <wayland-client.h>

// Vulkan
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

namespace vk {

class Instance;

class Surface
{
public:
    Surface(std::shared_ptr<Instance> instance,
            struct wl_display *wl_display, struct wl_surface *wl_surface);

    VkSurfaceKHR vk_surface();

private:
    struct wl_display *_wl_display;
    struct wl_surface *_wl_surface;
    VkSurfaceKHR _surface;
};

} // namespace vk

#endif // _SURFACE_H
