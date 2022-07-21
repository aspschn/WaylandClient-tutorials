#include "device.h"

// C
#include <stdio.h>
#include <string.h>

#include "instance.h"
#include "surface.h"

static const char *device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

namespace vk {

//=========================
// Device::QueueFamilies
//=========================

Device::QueueFamilies::QueueFamilies()
{
    // Init.
    this->_graphics_family_index = std::nullopt;
    this->_present_family_index = std::nullopt;
}

uint32_t Device::QueueFamilies::graphics_family_index() const
{
    if (this->_graphics_family_index == std::nullopt) {
        fprintf(stderr, "[WARN] Graphics family index is null!\n");
    }

    return this->_graphics_family_index.value();
}

uint32_t Device::QueueFamilies::present_family_index() const
{
    if (this->_present_family_index == std::nullopt) {
        fprintf(stderr, "[WARN] Present family index is null!\n");
    }

    return this->_present_family_index.value();
}

void Device::QueueFamilies::set_graphics_family_index(uint32_t index)
{
    this->_graphics_family_index = index;
}

void Device::QueueFamilies::set_present_family_index(uint32_t index)
{
    this->_present_family_index = index;
}

std::vector<uint32_t> Device::QueueFamilies::indices() const
{
    if (this->_graphics_family_index == std::nullopt ||
            this->_present_family_index == std::nullopt) {
        fprintf(stderr, "[WARN] Indices not set all.\n");
    }

    std::vector<uint32_t> v;
    v.push_back(this->_graphics_family_index.value());
    v.push_back(this->_present_family_index.value());

    return v;
}

//=============
// Device
//=============

Device::Device(std::shared_ptr<Instance> instance,
        std::shared_ptr<Surface> surface)
{
    // Init.
    this->_vk_device = nullptr;
    this->_queue_priority = 1.0f;
    this->_graphics_queue = nullptr;
    this->_present_queue = nullptr;
    memset(&this->_enabled_features, 0, sizeof(VkPhysicalDeviceFeatures));

    VkResult result;

    // Find queue families.
    uint32_t queue_families = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(instance->vk_physical_device(),
        &queue_families, NULL);
    fprintf(stderr, "Queue Families: %d\n", queue_families);

    auto *properties_list = new VkQueueFamilyProperties[queue_families];
    vkGetPhysicalDeviceQueueFamilyProperties(instance->vk_physical_device(),
        &queue_families, properties_list);
    for (uint32_t i = 0; i < queue_families; ++i) {
        VkQueueFamilyProperties properties = properties_list[i];
        fprintf(stderr, " - Queue count: %d\n", properties.queueCount);
        if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            fprintf(stderr, " -- Has queue graphics bit. index: %d\n", i);
            this->_queue_families.set_graphics_family_index(i);
        }

        // Presentation support.
        VkBool32 present_support = VK_FALSE;
        result = vkGetPhysicalDeviceSurfaceSupportKHR(
            instance->vk_physical_device(),
            i, surface->vk_surface(), &present_support);
        if (result != VK_SUCCESS) {
            fprintf(stderr, "Error!\n");
            delete[] properties_list;
            return;
        }
        if (present_support == VK_TRUE) {
            fprintf(stderr, "Present support. index: %d\n", i);
            this->_queue_families.set_present_family_index(i);
            break;
        }
    }
    delete[] properties_list;

    // Queue create infos.
    auto create_infos = new VkDeviceQueueCreateInfo[2];
    create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create_infos[0].queueFamilyIndex =
        this->_queue_families.graphics_family_index();
    create_infos[0].queueCount = 1;
    create_infos[0].pQueuePriorities = &this->_queue_priority;
    create_infos[0].flags = 0;
    create_infos[0].pNext = NULL;

    create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create_infos[1].queueFamilyIndex =
        this->_queue_families.present_family_index();
    create_infos[1].queueCount = 1;
    create_infos[1].pQueuePriorities = &this->_queue_priority;
    create_infos[1].flags = 0;
    create_infos[1].pNext = NULL;

    // Logical device.
    VkDeviceCreateInfo device_create_info;
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 2;
    device_create_info.pQueueCreateInfos = create_infos;

    device_create_info.pEnabledFeatures = &this->_enabled_features;

    device_create_info.enabledExtensionCount = 1;
    device_create_info.ppEnabledExtensionNames = device_extensions;

    // Zero or null.
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;
    device_create_info.flags = 0;
    device_create_info.pNext = NULL;

    result = vkCreateDevice(instance->vk_physical_device(),
        &device_create_info,
        NULL, &this->_vk_device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device! reuslt: %d\n",
            result);
        return;
    } else {
        fprintf(stderr, "Logical device created - device: %p\n", this->_vk_device);
    }

    vkGetDeviceQueue(this->_vk_device,
        this->_queue_families.graphics_family_index(),
        0,
        &this->_graphics_queue);
    vkGetDeviceQueue(this->_vk_device,
        this->_queue_families.present_family_index(),
        0,
        &this->_present_queue);
}

VkDevice Device::vk_device()
{
    return this->_vk_device;
}

uint32_t Device::graphics_queue_family_index() const
{
    return this->_queue_families.graphics_family_index();
}

uint32_t Device::present_queue_family_index() const
{
    return this->_queue_families.present_family_index();
}

VkQueue Device::graphics_queue() const
{
    return this->_graphics_queue;
}

VkQueue Device::present_queue() const
{
    return this->_present_queue;
}

} // namespace vk
