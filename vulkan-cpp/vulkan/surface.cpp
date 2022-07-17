#include "surface.h"

// C
#include <stdio.h>

#include "instance.h"

namespace vk {

Surface::Surface(std::shared_ptr<Instance> instance,
        struct wl_display *wl_display, struct wl_surface *wl_surface)
{
    // Init.
    this->_wl_display = wl_display;
    this->_wl_surface = wl_surface;
    this->_surface = nullptr;

    // Create.
    VkResult result;

    VkWaylandSurfaceCreateInfoKHR surface_create_info;
    surface_create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = NULL;
    surface_create_info.display = this->_wl_display;
    surface_create_info.surface = this->_wl_surface;

    result = vkCreateWaylandSurfaceKHR(instance->vk_instance(),
        &surface_create_info, NULL, &this->_surface);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create window surface! result: %d\n",
            result);
        return;
    }
    fprintf(stderr, "Vulkan surface created.\n");
}

VkSurfaceKHR Surface::vk_surface()
{
    return this->_surface;
}

} // namespace vk
