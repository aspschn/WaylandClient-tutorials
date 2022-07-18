#include "instance.h"

// C
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"

namespace vk {

Instance::Instance()
{
    VkResult result;

    // Init.
    this->_vk_instance = nullptr;
    this->_vk_extension_names = nullptr;
    this->_vk_physical_devices = nullptr;

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

    auto extension_properties = new VkExtensionProperties[extensions];
    vkEnumerateInstanceExtensionProperties(NULL, &extensions,
        extension_properties);

    this->_vk_extension_names = new const char*[extensions];
    for (uint32_t i = 0; i < extensions; ++i) {
        this->_vk_extension_names[i] = NULL;
    }

    for (uint32_t i = 0; i < extensions; ++i) {
        VkExtensionProperties properties = extension_properties[i];
        fprintf(stderr, " - Extension name: %s %p\n", properties.extensionName,
            properties.extensionName);
        this->_vk_extension_names[i] = new char[strlen(properties.extensionName) + 1];
        strcpy(
            (char*)this->_vk_extension_names[i],
            properties.extensionName);
    }
    delete[] extension_properties;

    // Create instance.
    VkInstanceCreateInfo instance_create_info;
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = NULL;
    instance_create_info.enabledExtensionCount = extensions;
    instance_create_info.ppEnabledExtensionNames = this->_vk_extension_names;

    // Nulls or zeros.
    instance_create_info.pApplicationInfo = nullptr;
    instance_create_info.flags = 0;
    instance_create_info.enabledLayerCount = 0;

    result = vkCreateInstance(&instance_create_info, NULL,
        &this->_vk_instance);

    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance - result: %d\n",
            result);
        return;
    } else {
        fprintf(stderr, "Vulkan instance created!\n");
    }

    // Pick physical device.
    uint32_t devices = 0;
    vkEnumeratePhysicalDevices(this->_vk_instance, &devices, NULL);
    if (devices == 0) {
        fprintf(stderr, "No GPUs with Vulkan support!\n");
        return;
    }
    fprintf(stderr, "Physical devices: %d\n", devices);

    this->_vk_physical_devices = new VkPhysicalDevice[devices];
    vkEnumeratePhysicalDevices(this->_vk_instance, &devices,
        this->_vk_physical_devices);

    for (uint32_t i = 0; i < devices; ++i) {
        VkPhysicalDevice device = this->_vk_physical_devices[i];
        fprintf(stderr, " - Device: %p\n", device);
        if (i == 0) {
            this->_vk_physical_device = device;
        }
    }
}

Instance::~Instance()
{
    //
}

VkInstance Instance::vk_instance()
{
    return this->_vk_instance;
}

VkPhysicalDevice Instance::vk_physical_device()
{
    return this->_vk_physical_device;
}

VkSurfaceCapabilitiesKHR Instance::surface_capabilities(
        VkSurfaceKHR vk_surface) const
{
    VkResult result;

    // Querying details of swap chain support.
    VkSurfaceCapabilitiesKHR capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        this->_vk_physical_device,
        vk_surface,
        &capabilities);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get surface capabilities!\n");
    }

    return capabilities;
}

std::vector<VkSurfaceFormatKHR> Instance::surface_formats(
        VkSurfaceKHR vk_surface) const
{
    VkResult result;

    std::vector<VkSurfaceFormatKHR> v;

    uint32_t formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(this->_vk_physical_device,
        vk_surface, &formats, NULL);
    fprintf(stderr, "Surface formats: %d\n", formats);

    auto surface_formats = new VkSurfaceFormatKHR[formats];
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(this->_vk_physical_device,
        vk_surface, &formats, surface_formats);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "[WARN] Failed to get surface formats!\n");
        delete[] surface_formats;

        return v;
    }

    for (uint32_t i = 0; i < formats; ++i) {
        VkSurfaceFormatKHR surface_format = surface_formats[i];
        fprintf(stderr, " - Format: %s, Color space: %d\n",
            vk_format_to_string(surface_format.format),
            surface_format.colorSpace);
        v.push_back(surface_format);
    }

    delete[] surface_formats;

    return v;
}

std::vector<VkPresentModeKHR> Instance::present_modes(
        VkSurfaceKHR vk_surface) const
{
    VkResult result;

    std::vector<VkPresentModeKHR> v;

    uint32_t modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(this->_vk_physical_device,
        vk_surface, &modes, NULL);
    if (modes == 0) {
        fprintf(stderr, "No surface present modes!\n");
        return v;
    }
    fprintf(stderr, "Surface present modes: %d\n", modes);

    auto present_modes = new VkPresentModeKHR[modes];

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        this->_vk_physical_device,
        vk_surface,
        &modes, present_modes);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get surface present modes!\n");
        delete[] present_modes;

        return v;
    }

    for (uint32_t i = 0; i < modes; ++i) {
        VkPresentModeKHR present_mode = present_modes[i];
        fprintf(stderr, " - Present mode: %s\n",
            vk_present_mode_to_string(present_mode));
        v.push_back(present_mode);
    }

    delete[] present_modes;

    return v;
}

} // namespace vk
