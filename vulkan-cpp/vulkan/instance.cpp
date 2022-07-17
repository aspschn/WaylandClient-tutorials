#include "instance.h"

// C
#include <stdint.h>
#include <string.h>
#include <stdio.h>

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

} // namespace vk
