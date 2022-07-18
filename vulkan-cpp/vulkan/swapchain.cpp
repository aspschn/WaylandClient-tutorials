#include "swapchain.h"

// C
#include <stdio.h>

// C++
#include <vector>

#include "instance.h"
#include "surface.h"
#include "device.h"

namespace vk {

Swapchain::Swapchain(std::shared_ptr<Instance> instance,
        std::shared_ptr<Surface> surface,
        std::shared_ptr<Device> device,
        uint32_t width, uint32_t height)
{
    // Init.
    this->_vk_swapchain = nullptr;
    this->_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    this->_image_count = 0;
    this->_extent.width = 0;
    this->_extent.height = 0;

    // Pick surface format.
    auto surface_formats = instance->surface_formats(surface->vk_surface());
    for (auto& format: surface_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            this->_surface_format = format;
            break;
        }
    }

    // Pick present mode.
    auto present_modes = instance->present_modes(surface->vk_surface());
    for (auto& mode: present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            this->_present_mode = mode;
            break;
        }
    }

    // Capabilities.
    auto capabilities = instance->surface_capabilities(surface->vk_surface());
    this->_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
            this->_image_count > capabilities.maxImageCount) {
        this->_image_count = capabilities.maxImageCount;
    }

    this->_extent.width = width;
    this->_extent.height = height;
    // TODO: Proper extent calculation code.

    // Create.
    VkResult result;

    uint32_t graphics_family = device->graphics_queue_family_index();
    uint32_t present_family = device->present_queue_family_index();
    auto queue_family_indices = new uint32_t[2];
    queue_family_indices[0] = graphics_family;
    queue_family_indices[1] = present_family;

    VkSwapchainCreateInfoKHR create_info;
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface->vk_surface();
    create_info.minImageCount = this->_image_count;
    create_info.imageFormat = this->_surface_format.format;
    create_info.imageColorSpace = this->_surface_format.colorSpace;
    create_info.imageExtent = this->_extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphics_family != present_family) {
        fprintf(stderr, "NOT SAME!\n");
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }
    create_info.preTransform = capabilities.currentTransform;
    // ?? No alpha?
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = this->_present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;
    fprintf(stderr, "Done writing swapchain create info.\n");

    result = vkCreateSwapchainKHR(device->vk_device(),
        &create_info, NULL, &this->_vk_swapchain);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain!\n");
        delete[] queue_family_indices;

        return;
    }
    fprintf(stderr, "Swapchain created!\n");

    delete[] queue_family_indices;
}

VkSwapchainKHR Swapchain::vk_swapchain()
{
    return this->_vk_swapchain;
}

} // namespace vk
