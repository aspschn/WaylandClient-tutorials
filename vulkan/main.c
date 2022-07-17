#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

#include <cairo.h>

#include "xdg-shell.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_subcompositor *subcompositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_output *output;

struct xdg_wm_base *xdg_wm_base = NULL;
struct xdg_surface *xdg_surface = NULL;
struct xdg_toplevel *xdg_toplevel = NULL;

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 360

// Vulkan validation layers.
const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};
#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif
VkLayerProperties *vulkan_layer_properties = NULL;
// Vulkan init.
VkInstance vulkan_instance = NULL;
VkInstanceCreateInfo vulkan_instance_create_info;
VkExtensionProperties *vulkan_extension_properties = NULL;
const char* *vulkan_extension_names = NULL;
VkPhysicalDevice vulkan_physical_device = VK_NULL_HANDLE;
VkPhysicalDevice *vulkan_physical_devices = NULL;
VkQueueFamilyProperties *vulkan_queue_families = NULL;
uint32_t graphics_family = 0;
VkDeviceQueueCreateInfo *vulkan_queue_create_infos = NULL;
float queue_priority = 1.0f;
VkPhysicalDeviceFeatures vulkan_device_features;
VkQueue vulkan_graphics_queue = NULL;
// Logical device.
VkDeviceCreateInfo vulkan_device_create_info;
VkDevice vulkan_device = NULL;
// Vulkan surface.
VkSurfaceKHR vulkan_surface = NULL;
VkWaylandSurfaceCreateInfoKHR vulkan_surface_create_info;
uint32_t present_family = 0;
VkQueue vulkan_present_queue = NULL;
// Swapchain.
uint32_t *queue_family_indices = NULL;
VkSurfaceCapabilitiesKHR vulkan_capabilities;
VkSurfaceFormatKHR *vulkan_formats = NULL;
VkPresentModeKHR *vulkan_present_modes = NULL;
const char* *vulkan_required_extension_names = NULL; // Not used.
VkSurfaceFormatKHR vulkan_format;
VkPresentModeKHR vulkan_present_mode;
VkExtent2D vulkan_extent;
VkSwapchainCreateInfoKHR vulkan_swapchain_create_info;
VkSwapchainKHR vulkan_swapchain = NULL;
const char *device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
VkImage *vulkan_swapchain_images = NULL;
uint32_t vulkan_swapchain_images_length = 0;
// Image views.
VkImageView *vulkan_image_views = NULL;
// Render pass.
VkAttachmentDescription vulkan_attachment_description;
VkAttachmentReference vulkan_attachment_reference;
VkSubpassDescription vulkan_subpass_description;
VkRenderPass vulkan_render_pass;
VkRenderPassCreateInfo vulkan_render_pass_create_info;
VkSubpassDependency vulkan_dependency;
// Shaders.
uint8_t *vert_shader_code = NULL;
uint32_t vert_shader_code_size = 0;
uint8_t *frag_shader_code = NULL;
uint32_t frag_shader_code_size = 0;
VkPipelineShaderStageCreateInfo vulkan_vert_shader_stage_create_info;
VkPipelineShaderStageCreateInfo vulkan_frag_shader_stage_create_info;
VkPipelineShaderStageCreateInfo *vulkan_shader_stage_create_infos = NULL;
VkPipelineVertexInputStateCreateInfo vulkan_vert_input_state_create_info;
VkPipelineInputAssemblyStateCreateInfo vulkan_input_assembly_state_create_info;
VkPipelineViewportStateCreateInfo vulkan_viewport_state_create_info;
VkPipelineRasterizationStateCreateInfo vulkan_rasterization_state_create_info;
VkPipelineMultisampleStateCreateInfo vulkan_multisample_state_create_info;
VkPipelineColorBlendAttachmentState vulkan_color_blend_attachment_state;
VkPipelineColorBlendStateCreateInfo vulkan_color_blend_state_create_info;
VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
};
VkPipelineDynamicStateCreateInfo vulkan_dynamic_state_create_info;
VkPipelineLayoutCreateInfo vulkan_layout_create_info;
VkPipelineLayout vulkan_layout = NULL;
VkGraphicsPipelineCreateInfo vulkan_graphics_pipeline_create_info;
VkPipeline vulkan_graphics_pipeline = NULL;
// Framebuffers.
VkFramebuffer *vulkan_framebuffers = NULL;
// Command pool.
VkCommandPoolCreateInfo vulkan_command_pool_create_info;
VkCommandPool vulkan_command_pool = NULL;
// Command buffer.
VkCommandBufferAllocateInfo vulkan_command_buffer_allocate_info;
VkCommandBuffer vulkan_command_buffer = NULL;
VkCommandBufferBeginInfo vulkan_command_buffer_begin_info; // Unused.
VkRenderPassBeginInfo vulkan_render_pass_begin_info;
VkClearValue vulkan_clear_color;
// Sync objects.
VkSemaphore vulkan_image_available_semaphore = NULL;
VkSemaphore vulkan_render_finished_semaphore = NULL;
VkFence vulkan_in_flight_fence = NULL;


struct wl_subsurface *subsurface;

uint32_t image_width;
uint32_t image_height;
uint32_t image_size;
uint32_t *image_data;

static const char* vk_format_to_string(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        return "VK_FORMAT_R5G6B5_UNORM_PACK16";
    case VK_FORMAT_R8G8B8A8_UNORM:
        return "VK_FORMAT_R8G8B8A8_UNORM";
    case VK_FORMAT_R8G8B8A8_SRGB:
        return "VK_FORMAT_R8G8B8A8_SRGB";
    case VK_FORMAT_B8G8R8A8_UNORM:
        return "VK_FORMAT_B8G8R8A8_UNORM";
    case VK_FORMAT_B8G8R8A8_SRGB:
        return "VK_FORMAT_B8G8R8A8_SRGB";
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
    default:
        return "Unknown";
    }
}

static const char* vk_present_mode_to_string(VkPresentModeKHR mode)
{
    switch (mode) {
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return "VK_PRESENT_MODE_MAILBOX_KHR";
    case VK_PRESENT_MODE_FIFO_KHR:
        return "VK_PRESENT_MODE_FIFO_KHR";
    default:
        return "Unknown";
    }
}

void load_image()
{
    cairo_surface_t *cairo_surface = cairo_image_surface_create_from_png(
        "miku@2x.png");
    cairo_t *cr = cairo_create(cairo_surface);
    image_width = cairo_image_surface_get_width(cairo_surface);
    image_height = cairo_image_surface_get_height(cairo_surface);
    image_size = sizeof(uint32_t) * (image_width * image_height);
    image_data = malloc(image_size);
    memcpy(image_data, cairo_image_surface_get_data(cairo_surface), image_size);

    // Color correct.
    for (uint32_t i = 0; i < (image_width * image_height); ++i) {
        uint32_t color = 0x00000000;
        color += ((image_data[i] & 0xff000000));  // Set alpha.
        color += ((image_data[i] & 0x00ff0000) >> 16);   // Set blue.
        color += ((image_data[i] & 0x0000ff00));   // Set green.
        color += ((image_data[i] & 0x000000ff) << 16);  // Set red.
        image_data[i] = color;
    }

    cairo_surface_destroy(cairo_surface);
    cairo_destroy(cr);
}

//===========
// Vulkan
//===========

static void load_shader(const char *path, uint8_t* *code, uint32_t *size)
{
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);

    *code = (uint8_t*)malloc(sizeof(uint8_t) * *size);
    fread(*code, *size, 1, f);

    fclose(f);
}

static void init_vulkan()
{
    VkResult result;

    // Check validation layers.
    uint32_t layers = 0;
    result = vkEnumerateInstanceLayerProperties(&layers, NULL);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Layer properties failed!\n");
        return;
    }
    fprintf(stderr, "Number of layers: %d\n", layers);


    // Check extensions.
    uint32_t extensions = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensions, NULL);
    fprintf(stderr, "Number of extensions: %d\n", extensions);

    vulkan_extension_properties = (VkExtensionProperties*)malloc(
        sizeof(VkExtensionProperties) * extensions
    );
    vkEnumerateInstanceExtensionProperties(NULL, &extensions,
        vulkan_extension_properties);

    vulkan_extension_names = (const char**)malloc(
        sizeof(const char*) * extensions
    );
    for (uint32_t i = 0; i < extensions; ++i) {
        *(vulkan_extension_names + i) = NULL;
    }

    for (uint32_t i = 0; i < extensions; ++i) {
        VkExtensionProperties properties = *(vulkan_extension_properties + i);
        fprintf(stderr, " - Extension name: %s %p\n", properties.extensionName,
            properties.extensionName);
        *(vulkan_extension_names + i) = (const char*)malloc(
            strlen(properties.extensionName + 1));
        strcpy(*(vulkan_extension_names + i), properties.extensionName);
    }

    // Create instance.
    vulkan_instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vulkan_instance_create_info.pNext = NULL;
    vulkan_instance_create_info.enabledExtensionCount = extensions;
    vulkan_instance_create_info.ppEnabledExtensionNames = vulkan_extension_names;

    result = vkCreateInstance(&vulkan_instance_create_info, NULL,
        &vulkan_instance);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance - result: %d\n",
            result);
        return;
    } else {
        fprintf(stderr, "Vulkan instance created!\n");
    }

    // Pick physical device.
    uint32_t devices = 0;
    vkEnumeratePhysicalDevices(vulkan_instance, &devices, NULL);
    if (devices == 0) {
        fprintf(stderr, "No GPUs with Vulkan support!\n");
        return;
    }
    fprintf(stderr, "Physical devices: %d\n", devices);

    vulkan_physical_devices = (VkPhysicalDevice*)malloc(
        sizeof(VkPhysicalDevice) * devices
    );
    vkEnumeratePhysicalDevices(vulkan_instance, &devices,
        vulkan_physical_devices);

    for (uint32_t i = 0; i < devices; ++i) {
        VkPhysicalDevice device = *(vulkan_physical_devices + i);
        fprintf(stderr, " - Device: %p\n", device);
        if (i == 0) {
            vulkan_physical_device = device;
        }
    }
}

static void create_vulkan_window()
{
    VkResult result;

    vulkan_surface_create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    vulkan_surface_create_info.pNext = NULL;
    vulkan_surface_create_info.display = display;
    vulkan_surface_create_info.surface = surface;

    result = vkCreateWaylandSurfaceKHR(vulkan_instance,
        &vulkan_surface_create_info, NULL, &vulkan_surface);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create window surface! result: %d\n",
            result);
        return;
    }
    fprintf(stderr, "Vulkan surface created.\n");
}

static void create_vulkan_logical_device()
{
    VkResult result;

    // Find queue families.
    uint32_t queue_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device,
        &queue_families, NULL);
    fprintf(stderr, "Queue Families: %d\n", queue_families);

    vulkan_queue_families = (VkQueueFamilyProperties*)malloc(
        sizeof(VkQueueFamilyProperties) * queue_families
    );
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device,
        &queue_families, vulkan_queue_families);

    for (uint32_t i = 0; i < queue_families; ++i) {
        VkQueueFamilyProperties properties = *(vulkan_queue_families + i);
        fprintf(stderr, " - Queue count: %d\n", properties.queueCount);
        if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            fprintf(stderr, " -- Has queue graphics bit. index: %d\n", i);
            graphics_family = i;
//            continue;
        }

        // Presentation support.
        VkBool32 present_support = VK_FALSE;
        result = vkGetPhysicalDeviceSurfaceSupportKHR(vulkan_physical_device,
            i, vulkan_surface, &present_support);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Error!\n");
            return;
        }
        if (present_support == VK_TRUE) {
            fprintf(stderr, "Present support. index: %d\n", i);
            present_family = i;
            break;
        }
    }
    queue_family_indices = (uint32_t*)malloc(sizeof(uint32_t) * 2);
    queue_family_indices[0] = graphics_family;
    queue_family_indices[1] = present_family;
    fprintf(stderr, "[DEBUG] graphics/present queue family indices: %d, %d\n",
        graphics_family,
        present_family);

    vulkan_queue_create_infos = (VkDeviceQueueCreateInfo*)malloc(
        sizeof(VkDeviceQueueCreateInfo) * 2
    );

    // Creating the presentation queue.
    vulkan_queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vulkan_queue_create_infos[0].queueFamilyIndex = graphics_family;
    vulkan_queue_create_infos[0].queueCount = 1;
    vulkan_queue_create_infos[0].pQueuePriorities = &queue_priority;
    vulkan_queue_create_infos[0].flags = 0;
    vulkan_queue_create_infos[0].pNext = NULL;

    vulkan_queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    vulkan_queue_create_infos[1].queueFamilyIndex = present_family;
    vulkan_queue_create_infos[1].queueCount = 1;
    vulkan_queue_create_infos[1].pQueuePriorities = &queue_priority;
    vulkan_queue_create_infos[1].flags = 0;
    vulkan_queue_create_infos[1].pNext = NULL;

    // Logical device.
    vulkan_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vulkan_device_create_info.queueCreateInfoCount = 2;
    vulkan_device_create_info.pQueueCreateInfos = vulkan_queue_create_infos;

    vulkan_device_create_info.pEnabledFeatures = &vulkan_device_features;

    vulkan_device_create_info.enabledExtensionCount = 1;
    vulkan_device_create_info.ppEnabledExtensionNames = device_extensions;

    // Zero or null.
    vulkan_device_create_info.enabledLayerCount = 0;
    vulkan_device_create_info.ppEnabledLayerNames = NULL;
    vulkan_device_create_info.flags = 0;
    vulkan_device_create_info.pNext = NULL;

    result = vkCreateDevice(vulkan_physical_device, &vulkan_device_create_info,
        NULL, &vulkan_device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device! reuslt: %d\n",
            result);
        return;
    } else {
        fprintf(stderr, "Logical device created - device: %p\n", vulkan_device);
    }

    vkGetDeviceQueue(vulkan_device, graphics_family, 0, &vulkan_graphics_queue);
    vkGetDeviceQueue(vulkan_device, present_family, 0, &vulkan_present_queue);

    // Querying details of swap chain support.
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_physical_device,
        vulkan_surface,
        &vulkan_capabilities);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get surface capabilities!\n");
        return;
    }
    fprintf(stderr, "Physical device surface capabilities. - transform: %d\n",
        vulkan_capabilities.currentTransform);

    uint32_t formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_physical_device,
        vulkan_surface, &formats, NULL);
    fprintf(stderr, "Surface formats: %d\n", formats);

    vulkan_formats = (VkSurfaceFormatKHR*)malloc(
        sizeof(VkSurfaceFormatKHR) * formats
    );
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(vulkan_physical_device,
        vulkan_surface, &formats, vulkan_formats);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get surface formats!\n");
        return;
    }

    for (uint32_t i = 0; i < formats; ++i) {
        VkSurfaceFormatKHR surface_format = *(vulkan_formats + i);
        fprintf(stderr, " - Format: %s, Color space: %d\n",
            vk_format_to_string(surface_format.format),
            surface_format.colorSpace);
        if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            vulkan_format = surface_format;
        }
    }

    uint32_t modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_physical_device,
        vulkan_surface, &modes, NULL);
    if (modes == 0) {
        fprintf(stderr, "No surface present modes!\n");
        return;
    }
    fprintf(stderr, "Surface present modes: %d\n", modes);

    vulkan_present_modes = (VkPresentModeKHR*)malloc(
        sizeof(VkPresentModeKHR) * modes
    );

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_physical_device,
        vulkan_surface, &modes, vulkan_present_modes);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get surface present modes!\n");
        return;
    }

    for (uint32_t i = 0; i < modes; ++i) {
        VkPresentModeKHR present_mode = *(vulkan_present_modes + i);
        fprintf(stderr, " - Present mode: %s\n",
            vk_present_mode_to_string(present_mode));
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            vulkan_present_mode = present_mode;
        }
    }

    // Swap extent.
    fprintf(stderr, "Current extent - %ux%u\n",
        vulkan_capabilities.currentExtent.width,
        vulkan_capabilities.currentExtent.height);
    fprintf(stderr, "Min image extent - %dx%d\n",
        vulkan_capabilities.minImageExtent.width,
        vulkan_capabilities.minImageExtent.height);
    fprintf(stderr, "Max image extent - %dx%d\n",
        vulkan_capabilities.maxImageExtent.width,
        vulkan_capabilities.maxImageExtent.height);

    vulkan_extent.width = WINDOW_WIDTH;
    vulkan_extent.height = WINDOW_HEIGHT;
}

static void create_vulkan_swapchain()
{
    VkResult result;

    fprintf(stderr, "Capabilities min, max image count: %d, %d\n",
        vulkan_capabilities.minImageCount,
        vulkan_capabilities.maxImageCount);
    uint32_t image_count = vulkan_capabilities.minImageCount + 1;
    fprintf(stderr, "Image count: %d\n", image_count);

    vulkan_swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vulkan_swapchain_create_info.surface = vulkan_surface;
    vulkan_swapchain_create_info.minImageCount = image_count;
    vulkan_swapchain_create_info.imageFormat = vulkan_format.format;
    vulkan_swapchain_create_info.imageColorSpace = vulkan_format.colorSpace;
    vulkan_swapchain_create_info.imageExtent = vulkan_extent;
    vulkan_swapchain_create_info.imageArrayLayers = 1;
    vulkan_swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphics_family != present_family) {
        fprintf(stderr, "NOT SAME!\n");
        vulkan_swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        vulkan_swapchain_create_info.queueFamilyIndexCount = 2;
        vulkan_swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        vulkan_swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vulkan_swapchain_create_info.queueFamilyIndexCount = 0;
        vulkan_swapchain_create_info.pQueueFamilyIndices = NULL;
    }
    vulkan_swapchain_create_info.preTransform = vulkan_capabilities.currentTransform;
    // ?? No alpha?
    vulkan_swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vulkan_swapchain_create_info.presentMode = vulkan_present_mode;
    vulkan_swapchain_create_info.clipped = VK_TRUE;
    vulkan_swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    fprintf(stderr, "Done writing swapchain create info.\n");

    result = vkCreateSwapchainKHR(vulkan_device,
        &vulkan_swapchain_create_info, NULL, &vulkan_swapchain);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create swapchain!\n");
        return;
    }
    fprintf(stderr, "Swapchain created!\n");

    // Images.
    uint32_t images;
    result = vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &images, NULL);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get number of swapchain images!\n");
        return;
    }
    fprintf(stderr, "Number of images: %d\n", images);

    vulkan_swapchain_images = (VkImage*)malloc(
        sizeof(VkImage) * images
    );
    result = vkGetSwapchainImagesKHR(vulkan_device, vulkan_swapchain, &images,
        vulkan_swapchain_images);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get swapchain images!\n");
        return;
    }
    fprintf(stderr, "Swapchain images got.\n");
    vulkan_swapchain_images_length = images;
}

static void create_vulkan_image_views()
{
    VkResult result;

    vulkan_image_views = (VkImageView*)malloc(
        sizeof(VkImageView) * vulkan_swapchain_images_length
    );
    for (uint32_t i = 0; i < vulkan_swapchain_images_length; ++i) {
        VkImageViewCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = vulkan_swapchain_images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = vulkan_format.format;

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

        result = vkCreateImageView(vulkan_device, &create_info, NULL,
            &vulkan_image_views[i]);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Image view creation failed. index: %d\n", i);
        }
        fprintf(stderr, "Image view created - image view: %p\n", vulkan_image_views[i]);
    }
}

static void create_vulkan_render_pass()
{
    VkResult result;

    vulkan_attachment_description.format = vulkan_format.format;
    vulkan_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    vulkan_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    vulkan_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    vulkan_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    vulkan_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    vulkan_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vulkan_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vulkan_attachment_reference.attachment = 0;
    vulkan_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    vulkan_subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    vulkan_subpass_description.colorAttachmentCount = 1;
    vulkan_subpass_description.pColorAttachments = &vulkan_attachment_reference;

    // Subpass dependency.
    vulkan_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    vulkan_dependency.dstSubpass = 0;
    vulkan_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vulkan_dependency.srcAccessMask = 0;
    vulkan_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    vulkan_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vulkan_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    vulkan_render_pass_create_info.attachmentCount = 1;
    vulkan_render_pass_create_info.pAttachments = &vulkan_attachment_description;
    vulkan_render_pass_create_info.subpassCount = 1;
    vulkan_render_pass_create_info.pSubpasses = &vulkan_subpass_description;
    vulkan_render_pass_create_info.dependencyCount = 1;
    vulkan_render_pass_create_info.pDependencies = &vulkan_dependency;

    result = vkCreateRenderPass(vulkan_device, &vulkan_render_pass_create_info,
        NULL, &vulkan_render_pass);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass!\n");
    }
    fprintf(stderr, "Render pass created. - render pass: %p\n",
        vulkan_render_pass);
}

static void create_vulkan_graphics_pipeline()
{
    VkResult result;

    // Load shader codes.
    load_shader("vert.spv", &vert_shader_code, &vert_shader_code_size);
    load_shader("frag.spv", &frag_shader_code, &frag_shader_code_size);

    // Create vertex shader module.
    VkShaderModule vert_shader_module;

    VkShaderModuleCreateInfo vert_create_info;
    vert_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_create_info.codeSize = vert_shader_code_size;
    vert_create_info.pCode = (const uint32_t*)vert_shader_code;
    vert_create_info.flags = 0;
    vert_create_info.pNext = NULL;

    result = vkCreateShaderModule(vulkan_device, &vert_create_info, NULL,
        &vert_shader_module);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex shader module!\n");
        return;
    }
    fprintf(stderr, "Vertex shader module created.\n");

    // Create fragment shader module.
    VkShaderModule frag_shader_module;

    VkShaderModuleCreateInfo frag_create_info;
    frag_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_create_info.codeSize = frag_shader_code_size;
    frag_create_info.pCode = (const uint32_t*)frag_shader_code;
    frag_create_info.flags = 0;
    frag_create_info.pNext = NULL;

    result = vkCreateShaderModule(vulkan_device, &frag_create_info, NULL,
        &frag_shader_module);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create fragment shader module!\n");
        return;
    }
    fprintf(stderr, "Fragment shader module created.\n");

    // Write shader stage create infos.
    vulkan_vert_shader_stage_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vulkan_vert_shader_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vulkan_vert_shader_stage_create_info.module = vert_shader_module;
    vulkan_vert_shader_stage_create_info.pName = "main";

    vulkan_frag_shader_stage_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vulkan_frag_shader_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vulkan_frag_shader_stage_create_info.module = frag_shader_module;
    vulkan_frag_shader_stage_create_info.pName = "main";

    // Shader stage.
    vulkan_shader_stage_create_infos = (VkPipelineShaderStageCreateInfo*)malloc(
        sizeof(VkPipelineShaderStageCreateInfo) * 2
    );
    *(vulkan_shader_stage_create_infos + 0) = vulkan_vert_shader_stage_create_info;
    *(vulkan_shader_stage_create_infos + 1) = vulkan_frag_shader_stage_create_info;

    // Vertex input.
    vulkan_vert_input_state_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vulkan_vert_input_state_create_info.vertexBindingDescriptionCount = 0;
    vulkan_vert_input_state_create_info.vertexAttributeDescriptionCount = 0;

    // Input assembly.
    vulkan_input_assembly_state_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    vulkan_input_assembly_state_create_info.topology =
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vulkan_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    // Viewport.
    vulkan_viewport_state_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vulkan_viewport_state_create_info.viewportCount = 1;
    vulkan_viewport_state_create_info.scissorCount = 1;

    // Rasterization.
    vulkan_rasterization_state_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    vulkan_rasterization_state_create_info.depthClampEnable = VK_FALSE;
    vulkan_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    vulkan_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    vulkan_rasterization_state_create_info.lineWidth = 1.0f;
    vulkan_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    vulkan_rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    vulkan_rasterization_state_create_info.depthBiasEnable = VK_FALSE;

    // Multisampling.
    vulkan_multisample_state_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    vulkan_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    vulkan_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blend attachment.
    vulkan_color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    vulkan_color_blend_attachment_state.blendEnable = VK_FALSE;

    // Color blending.
    vulkan_color_blend_state_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    vulkan_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    vulkan_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    vulkan_color_blend_state_create_info.attachmentCount = 1;
    vulkan_color_blend_state_create_info.pAttachments =
        &vulkan_color_blend_attachment_state;
    vulkan_color_blend_state_create_info.blendConstants[0] = 0.0f;
    vulkan_color_blend_state_create_info.blendConstants[1] = 0.0f;
    vulkan_color_blend_state_create_info.blendConstants[2] = 0.0f;
    vulkan_color_blend_state_create_info.blendConstants[3] = 0.0f;

    // Dynamic states.
    vulkan_dynamic_state_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    vulkan_dynamic_state_create_info.dynamicStateCount = 2;
    vulkan_dynamic_state_create_info.pDynamicStates = dynamic_states;

    // Layout.
    vulkan_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    vulkan_layout_create_info.setLayoutCount = 0;
    vulkan_layout_create_info.pushConstantRangeCount = 0;

    result = vkCreatePipelineLayout(vulkan_device,
        &vulkan_layout_create_info, NULL, &vulkan_layout);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout!\n");
        return;
    }
    fprintf(stderr, "Pipeline layout created.\n");

    vulkan_graphics_pipeline_create_info.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    vulkan_graphics_pipeline_create_info.stageCount = 2;
    vulkan_graphics_pipeline_create_info.pStages = vulkan_shader_stage_create_infos;
    vulkan_graphics_pipeline_create_info.pVertexInputState =
        &vulkan_vert_input_state_create_info;
    vulkan_graphics_pipeline_create_info.pInputAssemblyState =
        &vulkan_input_assembly_state_create_info;
    vulkan_graphics_pipeline_create_info.pViewportState =
        &vulkan_viewport_state_create_info;
    vulkan_graphics_pipeline_create_info.pRasterizationState =
        &vulkan_rasterization_state_create_info;
    vulkan_graphics_pipeline_create_info.pMultisampleState =
        &vulkan_multisample_state_create_info;
    vulkan_graphics_pipeline_create_info.pColorBlendState =
        &vulkan_color_blend_state_create_info;
    vulkan_graphics_pipeline_create_info.pDynamicState =
        &vulkan_dynamic_state_create_info;
    vulkan_graphics_pipeline_create_info.layout = vulkan_layout;
    vulkan_graphics_pipeline_create_info.renderPass = vulkan_render_pass;
    vulkan_graphics_pipeline_create_info.subpass = 0;
    vulkan_graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

    result = vkCreateGraphicsPipelines(vulkan_device, VK_NULL_HANDLE,
        1, &vulkan_graphics_pipeline_create_info, NULL,
        &vulkan_graphics_pipeline);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline!\n");
        return;
    }
    fprintf(stderr, "Graphics pipeline created - pipeline: %p\n",
        vulkan_graphics_pipeline);

    /*
    vkDestroyShaderModule(vulkan_device, frag_shader_module, NULL);
    vkDestroyShaderModule(vulkan_device, vert_shader_module, NULL);
    */
}

static void create_vulkan_framebuffers()
{
    VkResult result;

    vulkan_framebuffers = (VkFramebuffer*)malloc(
        sizeof(VkFramebuffer) * vulkan_swapchain_images_length
    );

    for (uint32_t i = 0; i < vulkan_swapchain_images_length; ++i) {
        VkImageView attachments[] = {
            vulkan_image_views[i],
        };

        VkFramebufferCreateInfo create_info;
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = vulkan_render_pass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = vulkan_extent.width;
        create_info.height = vulkan_extent.height;
        create_info.layers = 1;
        create_info.flags = 0;
        create_info.pNext = NULL;

        result = vkCreateFramebuffer(vulkan_device, &create_info, NULL,
            (vulkan_framebuffers + i));
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer.\n");
            return;
        }
        fprintf(stderr, "Framebuffer created. - framebuffer: %p\n",
            vulkan_framebuffers[i]);
    }
}

static void create_vulkan_command_pool()
{
    VkResult result;

    vulkan_command_pool_create_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    vulkan_command_pool_create_info.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vulkan_command_pool_create_info.queueFamilyIndex = graphics_family;

    result = vkCreateCommandPool(vulkan_device,
        &vulkan_command_pool_create_info, NULL, &vulkan_command_pool);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool!\n");
        return;
    }
    fprintf(stderr, "Command pool created. - command pool: %p\n",
        vulkan_command_pool);
}

static void create_vulkan_command_buffer()
{
    VkResult result;

    vulkan_command_buffer_allocate_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    vulkan_command_buffer_allocate_info.commandPool = vulkan_command_pool;
    vulkan_command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vulkan_command_buffer_allocate_info.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(vulkan_device,
        &vulkan_command_buffer_allocate_info, &vulkan_command_buffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers!\n");
    }
    fprintf(stderr, "Command buffer allocated. - command buffer: %p\n",
        vulkan_command_buffer);
}

static void create_vulkan_sync_objects()
{
    VkResult result;

    VkSemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.flags = 0;
    semaphore_create_info.pNext = NULL;

    VkFenceCreateInfo fence_create_info;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    result = vkCreateSemaphore(vulkan_device, &semaphore_create_info,
        NULL, &vulkan_image_available_semaphore);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image available semaphore!\n");
        return;
    }
    fprintf(stderr, "Image available semaphore created.\n");
    result = vkCreateSemaphore(vulkan_device, &semaphore_create_info,
        NULL, &vulkan_render_finished_semaphore);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render finished semaphore!\n");
        return;
    }
    fprintf(stderr, "Render finished semaphore created.\n");
    result = vkCreateFence(vulkan_device, &fence_create_info,
        NULL, &vulkan_in_flight_fence);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create fence!\n");
        return;
    }
    fprintf(stderr, "Fence created.\n");
}

static void record_command_buffer(VkCommandBuffer command_buffer,
        uint32_t image_index)
{
    VkResult result;

    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = 0;
    command_buffer_begin_info.pInheritanceInfo = NULL;
    command_buffer_begin_info.pNext = NULL;

    result = vkBeginCommandBuffer(command_buffer,
        &command_buffer_begin_info);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin recording command buffer!\n");
        return;
    }
    fprintf(stderr, "Begin command buffer.\n");
    fprintf(stderr, "Using framebuffer: %p\n", vulkan_framebuffers[image_index]);

    vulkan_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vulkan_render_pass_begin_info.renderPass = vulkan_render_pass;
    vulkan_render_pass_begin_info.framebuffer = vulkan_framebuffers[image_index];
    vulkan_render_pass_begin_info.renderArea.offset.x = 0;
    vulkan_render_pass_begin_info.renderArea.offset.y = 0;
    vulkan_render_pass_begin_info.renderArea.extent = vulkan_extent;

    vulkan_clear_color.color.float32[0] = 0.0f;
    vulkan_clear_color.color.float32[1] = 0.0f;
    vulkan_clear_color.color.float32[2] = 0.0f;
    vulkan_clear_color.color.float32[3] = 1.0f;

    vulkan_render_pass_begin_info.clearValueCount = 1;
    vulkan_render_pass_begin_info.pClearValues = &vulkan_clear_color;

    fprintf(stderr, "vkCmdBeginRenderPass() - command buffer: %p\n",
        command_buffer);
    vkCmdBeginRenderPass(command_buffer, &vulkan_render_pass_begin_info,
        VK_SUBPASS_CONTENTS_INLINE);
    fprintf(stderr, "Begin render pass.\n");

    //==============
    // In Commands
    //==============
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        vulkan_graphics_pipeline);

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkan_extent.width;
    viewport.height = (float)vulkan_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = vulkan_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    //===============
    // Out Commands
    //===============

    vkCmdEndRenderPass(command_buffer);

    result = vkEndCommandBuffer(command_buffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to record command buffer!\n");
        return;
    }
    fprintf(stderr, "End command buffer.\n");
}

void draw_frame()
{
    VkResult result;

    vkWaitForFences(vulkan_device, 1, &vulkan_in_flight_fence,
        VK_TRUE, UINT64_MAX);
    vkResetFences(vulkan_device, 1, &vulkan_in_flight_fence);

    uint32_t image_index;
    result = vkAcquireNextImageKHR(vulkan_device, vulkan_swapchain, UINT64_MAX,
        vulkan_image_available_semaphore, VK_NULL_HANDLE, &image_index);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to acquire next image!\n");
        return;
    }
    fprintf(stderr, "Acquired next image. - image index: %d\n", image_index);

    vkResetCommandBuffer(vulkan_command_buffer, 0);
    record_command_buffer(vulkan_command_buffer, image_index);

    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {
        vulkan_image_available_semaphore,
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vulkan_command_buffer;

    VkSemaphore signal_semaphores[] = {
        vulkan_render_finished_semaphore,
    };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    result = vkQueueSubmit(vulkan_graphics_queue, 1, &submit_info,
        vulkan_in_flight_fence);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to submit draw command buffer!\n");
        return;
    }

    VkPresentInfoKHR present_info;
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {
        vulkan_swapchain,
    };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;

    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(vulkan_present_queue, &present_info);
}

//===========
// XDG
//===========
static void xdg_wm_base_ping_handler(void *data,
        struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler,
};

static void xdg_surface_configure_handler(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    fprintf(stderr, "xdg_surface_configure_handler()\n");
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

static void xdg_toplevel_configure_handler(void *data,
        struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
}

static void xdg_toplevel_close_handler(void *data,
        struct xdg_toplevel *xdg_toplevel)
{
}

const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

//==============
// Output
//==============

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(
            registry,
            id,
            &wl_compositor_interface,
            version
        );
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        xdg_wm_base = wl_registry_bind(registry,
            id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
    } else {
        printf("(%d) Got a registry event for <%s> id <%d>\n",
            version, interface, id);
    }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
        uint32_t id)
{
    printf("Got a registry losing event for <%d>\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display.\n");
        exit(1);
    }
    printf("Connected to display.\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_roundtrip(display);
    fprintf(stderr, "wl_display_roundtrip()\n");

    if (compositor == NULL || xdg_wm_base == NULL) {
        fprintf(stderr, "Can't find compositor or xdg_wm_base.\n");
        exit(1);
    }

    // Check surface.
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        fprintf(stderr, "Can't create surface.\n");
        exit(1);
    }

    // Check xdg surface.
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
    if (xdg_surface == NULL) {
        fprintf(stderr, "Can't create xdg surface.\n");
        exit(1);
    }
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    if (xdg_toplevel == NULL) {
        exit(1);
    }
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);


    // MUST COMMIT! or not working on weston.
    wl_surface_commit(surface);

    wl_display_roundtrip(display);

    // Vulkan init.
    init_vulkan();
    create_vulkan_window();
    create_vulkan_logical_device();
    create_vulkan_swapchain();
    create_vulkan_image_views();
    create_vulkan_render_pass();
    create_vulkan_graphics_pipeline();
    create_vulkan_framebuffers();
    create_vulkan_command_pool();
    create_vulkan_command_buffer();
    create_vulkan_sync_objects();

    draw_frame();

    wl_surface_commit(surface);

    int res = wl_display_dispatch(display);
    fprintf(stderr, "Initial dispatch.\n");
    while (res != -1) {
        res = wl_display_dispatch(display);
    }
    fprintf(stderr, "wl_display_dispatch() - res: %d\n", res);

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
