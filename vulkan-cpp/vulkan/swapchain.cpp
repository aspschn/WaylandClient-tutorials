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
        VkRenderPass render_pass,
        uint32_t width, uint32_t height)
{
    // Init.
    this->_device = device;
    this->_render_pass = render_pass;

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
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
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

    // Create images.
    this->_create_images(device->vk_device());

    // Create image views.
    this->_create_image_views(device->vk_device());

    // Create framebuffers.
    this->_create_framebuffers();
}

VkSwapchainKHR Swapchain::vk_swapchain()
{
    return this->_vk_swapchain;
}

VkSurfaceFormatKHR Swapchain::surface_format() const
{
    return this->_surface_format;
}

std::vector<VkImage> Swapchain::images() const
{
    return this->_images;
}

std::vector<VkImageView> Swapchain::image_views() const
{
    return this->_image_views;
}

std::vector<VkFramebuffer> Swapchain::framebuffers() const
{
    return this->_framebuffers;
}

VkExtent2D Swapchain::extent() const
{
    return this->_extent;
}

//==================
// Private Methods
//==================
void Swapchain::_create_images(VkDevice vk_device)
{
    VkResult result;

    uint32_t images;
    result = vkGetSwapchainImagesKHR(vk_device,
        this->_vk_swapchain, &images, NULL);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get number of swapchain images!\n");
        return;
    }
    fprintf(stderr, "Number of images: %d\n", images);

    auto swapchain_images = new VkImage[images];
    result = vkGetSwapchainImagesKHR(vk_device,
        this->_vk_swapchain, &images, swapchain_images);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get swapchain images!\n");
        delete[] swapchain_images;

        return;
    }
    fprintf(stderr, "Swapchain images got.\n");
    this->_image_count = images;

    for (uint32_t i = 0; i < images; ++i) {
        this->_images.push_back(swapchain_images[i]);
    }

    delete[] swapchain_images;
}

void Swapchain::_destroy_images()
{
    //
}

void Swapchain::_create_image_views(VkDevice vk_device)
{
    VkResult result;

    auto images = this->images();

    auto image_views = new VkImageView[images.size()];
    for (uint32_t i = 0; i < images.size(); ++i) {
        VkImageViewCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = this->surface_format().format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        create_info.flags = 0;
        create_info.pNext = NULL;

        result = vkCreateImageView(vk_device, &create_info, NULL,
            &image_views[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Image view creation failed. index: %d\n", i);
        }
        fprintf(stderr, "Image view created - image view: %p\n", image_views[i]);
        this->_image_views.push_back(image_views[i]);
    }

    delete[] image_views;
}

void Swapchain::_destroy_image_views(VkDevice vk_device)
{
    for (uint64_t i = 0; i < this->_image_views.size(); ++i) {
        vkDestroyImageView(vk_device, this->_image_views[i], NULL);
    }
    this->_image_views.clear();
}

void Swapchain::_create_framebuffers()
{
    VkResult result;

    auto image_views = this->image_views();

    auto framebuffers = new VkFramebuffer[image_views.size()];

    for (uint32_t i = 0; i < image_views.size(); ++i) {
        VkImageView attachments[] = {
            image_views[i],
        };

        VkFramebufferCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = this->_render_pass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = this->extent().width;
        create_info.height = this->extent().height;
        create_info.layers = 1;
        create_info.flags = 0;
        create_info.pNext = NULL;

        result = vkCreateFramebuffer(this->_device->vk_device(), &create_info,
            NULL,
            &framebuffers[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer.\n");
            delete[] framebuffers;

            return;
        }
        fprintf(stderr, "Framebuffer created. - framebuffer: %p\n",
            framebuffers[i]);
        this->_framebuffers.push_back(framebuffers[i]);
    }

    delete[] framebuffers;
}

void Swapchain::_destroy_framebuffers()
{
    for (uint64_t i = 0; i < this->_framebuffers.size(); ++i) {
        vkDestroyFramebuffer(this->_device->vk_device(),
            this->_framebuffers[i],
            NULL);
    }
    this->_framebuffers.clear();
}

} // namespace vk
